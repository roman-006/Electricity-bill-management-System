/* =============================================================================
   Electricity Billing & Consumer Management System — web edition
   A client-side port of the original console C++ project. All data lives in
   this browser's localStorage, mirroring the original's flat-file storage.
   ========================================================================== */

const STORAGE_KEY = "ebs_data_v1";

/* ---------------------------------------------------------------------------
   Simple non-cryptographic hash — same spirit as the original's use of
   std::hash<string>: keeps the raw password out of storage without pulling
   in a real crypto library. Not meant to be secure.
   ------------------------------------------------------------------------ */
function hashPassword(pw) {
  let h = 5381;
  for (let i = 0; i < pw.length; i++) {
    h = ((h << 5) + h + pw.charCodeAt(i)) >>> 0;
  }
  return h.toString(16);
}

/* ---------------------------------------------------------------------------
   Tariff calculators — ported line-for-line from Tariff.h
   ------------------------------------------------------------------------ */
const Tariff = {
  Residential(units) {
    let amount = 50.0;
    if (units <= 50) amount += units * 5.0;
    else if (units <= 100) amount += 50 * 5.0 + (units - 50) * 7.0;
    else amount += 50 * 5.0 + 50 * 7.0 + (units - 100) * 10.0;
    return amount;
  },
  Commercial(units) {
    let amount = 150.0 + units * 10.0;
    if (units > 200) amount += (units - 200) * 2.0;
    return amount;
  },
  Industrial(units) {
    let amount = 300.0 + units * 12.0;
    if (units > 500) amount += (units - 500) * 3.0;
    return amount;
  }
};

/* ---------------------------------------------------------------------------
   State — default/empty dataset, loaded from / saved to localStorage
   ------------------------------------------------------------------------ */
let state = null;

function defaultState() {
  return {
    consumerCount: 0,
    billCounter: 0,
    admin: { user: "admin", hash: hashPassword("admin123") },
    consumers: [],   // {id,name,address,category,meter:{previousReading,currentReading},billHistory:[],outstandingBalance}
    backups: []      // {label, timestamp, snapshot}
  };
}

function loadState() {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    if (raw) return JSON.parse(raw);
  } catch (e) { /* fall through to default */ }
  return defaultState();
}

function saveState() {
  localStorage.setItem(STORAGE_KEY, JSON.stringify(state));
}

/* ---------------------------------------------------------------------------
   Model helpers
   ------------------------------------------------------------------------ */
function findConsumer(id) {
  return state.consumers.find(c => c.id === id) || null;
}

function unitsConsumed(c) {
  return c.meter.currentReading - c.meter.previousReading;
}

function calculateBill(c) {
  return Tariff[c.category](unitsConsumed(c));
}

function makeConsumer(name, address, category, previousReading) {
  state.consumerCount += 1;
  return {
    id: state.consumerCount,
    name, address, category,
    meter: { previousReading, currentReading: previousReading },
    billHistory: [],
    outstandingBalance: 0
  };
}

function generateBillFor(c, date) {
  const units = unitsConsumed(c);
  const amount = calculateBill(c);
  state.billCounter += 1;
  const bill = {
    billId: state.billCounter,
    consumerId: c.id,
    consumerName: c.name,
    category: c.category,
    unitsConsumed: units,
    amount,
    date,
    paid: false,
    amountPaid: 0
  };
  c.billHistory.push(bill);
  c.outstandingBalance += amount;
  c.meter.previousReading = c.meter.currentReading;
  return bill;
}

function recordPaymentFor(c, amount) {
  c.outstandingBalance -= amount;
  if (c.outstandingBalance < 0) c.outstandingBalance = 0;
  let remaining = amount;
  for (const b of c.billHistory) {
    if (remaining <= 0) break;
    if (!b.paid) {
      const due = b.amount - b.amountPaid;
      const pay = Math.min(due, remaining);
      b.amountPaid += pay;
      if (b.amountPaid >= b.amount) b.paid = true;
      remaining -= pay;
    }
  }
}

/* ---------------------------------------------------------------------------
   Formatting helpers
   ------------------------------------------------------------------------ */
const rs = n => "Rs. " + Number(n).toFixed(2);
const todayStr = () => new Date().toISOString().slice(0, 10);
function esc(s) {
  return String(s).replace(/[&<>"']/g, m => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[m]));
}
function badge(category) {
  const cls = { Residential: "badge-residential", Commercial: "badge-commercial", Industrial: "badge-industrial" }[category];
  return `<span class="badge ${cls}">${category}</span>`;
}
function statusPill(paid) {
  return paid ? `<span class="status-pill status-paid">PAID</span>` : `<span class="status-pill status-due">DUE</span>`;
}

/* ---------------------------------------------------------------------------
   Toast
   ------------------------------------------------------------------------ */
let toastTimer = null;
function toast(msg, isError) {
  const el = document.getElementById("toast");
  el.textContent = msg;
  el.classList.toggle("error", !!isError);
  el.classList.remove("hidden");
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => el.classList.add("hidden"), 3200);
}

/* =============================================================================
   AUTH / LOGIN
   ========================================================================== */
const loginScreen = document.getElementById("loginScreen");
const appShell = document.getElementById("appShell");

function ledSequence() {
  const leds = document.querySelectorAll(".switch-led");
  leds.forEach((l, i) => setTimeout(() => l.classList.add(i === 2 ? "good" : "on"), i * 220));
}

document.getElementById("loginForm").addEventListener("submit", e => {
  e.preventDefault();
  const user = document.getElementById("loginUser").value.trim();
  const pass = document.getElementById("loginPass").value;
  const notice = document.getElementById("loginNotice");

  if (user === state.admin.user && hashPassword(pass) === state.admin.hash) {
    notice.classList.add("hidden");
    document.getElementById("currentUser").textContent = state.admin.user;
    loginScreen.classList.add("hidden");
    appShell.classList.remove("hidden");
    switchView("dashboard");
  } else {
    notice.textContent = "Incorrect username or password.";
    notice.classList.remove("hidden");
    notice.className = "notice error";
  }
});

document.getElementById("logoutBtn").addEventListener("click", () => {
  appShell.classList.add("hidden");
  loginScreen.classList.remove("hidden");
  document.getElementById("loginForm").reset();
});

document.getElementById("passwordForm").addEventListener("submit", e => {
  e.preventDefault();
  const user = document.getElementById("pwUser").value.trim();
  const oldPass = document.getElementById("pwOld").value;
  const newPass = document.getElementById("pwNew").value;

  if (user !== state.admin.user || hashPassword(oldPass) !== state.admin.hash) {
    toast("Username/password did not match — password not changed.", true);
    return;
  }
  if (newPass.length < 4) {
    toast("New password must be at least 4 characters.", true);
    return;
  }
  state.admin.hash = hashPassword(newPass);
  saveState();
  e.target.reset();
  toast("Password changed successfully.");
});

/* =============================================================================
   NAVIGATION
   ========================================================================== */
const VIEW_META = {
  dashboard: ["Dashboard", "Overview of the whole system"],
  consumers: ["Consumers", "Register, search, sort, and manage consumers"],
  meter: ["Meter Reading", "Update a consumer's current meter reading"],
  billing: ["Generate Bill", "Bill a consumer for units consumed this cycle"],
  payment: ["Record Payment", "Apply a payment against outstanding dues"],
  receipts: ["Receipts", "Print or reprint a receipt for any past bill"],
  reports: ["Reports & Statistics", "Revenue, dues, and top-consumer summaries"],
  backup: ["Backup & Restore", "Snapshot or restore the dataset"],
  admin: ["Admin Account", "Change the admin password"]
};

function switchView(name) {
  document.querySelectorAll(".nav-switch").forEach(b => b.classList.toggle("active", b.dataset.view === name));
  document.querySelectorAll(".view").forEach(v => v.classList.add("hidden"));
  document.getElementById("view-" + name).classList.remove("hidden");
  document.getElementById("viewTitle").textContent = VIEW_META[name][0];
  document.getElementById("viewSubtitle").textContent = VIEW_META[name][1];
  renderView(name);
}

document.getElementById("panelNav").addEventListener("click", e => {
  const btn = e.target.closest(".nav-switch");
  if (btn) switchView(btn.dataset.view);
});

function renderView(name) {
  if (name === "dashboard") renderDashboard();
  if (name === "consumers") renderConsumerTable();
  if (name === "meter") renderMeterView();
  if (name === "billing") renderBillingView();
  if (name === "payment") renderPaymentView();
  if (name === "receipts") renderReceiptsView();
  if (name === "reports") renderReport(currentReportTab);
  if (name === "backup") renderBackupView();
}

function refreshAllSelects() {
  populateConsumerSelect("meterConsumer");
  populateConsumerSelect("billConsumer");
  populateConsumerSelect("paymentConsumer");
  populateConsumerSelect("receiptConsumer");
}

function populateConsumerSelect(id) {
  const sel = document.getElementById(id);
  const prev = sel.value;
  sel.innerHTML = `<option value="">Select a consumer&hellip;</option>` +
    state.consumers.map(c => `<option value="${c.id}">#${c.id} — ${esc(c.name)} (${c.category})</option>`).join("");
  if (prev) sel.value = prev;
}

/* =============================================================================
   DASHBOARD
   ========================================================================== */
function renderDashboard() {
  let residential = 0, commercial = 0, industrial = 0;
  let totalBilled = 0, totalCollected = 0, totalDues = 0, billsIssued = 0, unpaid = 0;

  for (const c of state.consumers) {
    if (c.category === "Residential") residential++;
    else if (c.category === "Commercial") commercial++;
    else industrial++;
    totalDues += c.outstandingBalance;
    for (const b of c.billHistory) {
      totalBilled += b.amount;
      totalCollected += b.amountPaid;
      billsIssued++;
      if (!b.paid) unpaid++;
    }
  }

  const stats = [
    ["Total consumers", state.consumers.length, `${residential} residential · ${commercial} commercial · ${industrial} industrial`],
    ["Bills issued", billsIssued, `${unpaid} unpaid`],
    ["Revenue collected", rs(totalCollected), "sum of payments received"],
    ["Pending dues", rs(totalDues), "outstanding across all consumers"]
  ];
  document.getElementById("statGrid").innerHTML = stats.map(([label, value, sub]) => `
    <div class="stat-card">
      <div class="label">${label}</div>
      <div class="value">${value}</div>
      <div class="sub">${sub}</div>
    </div>`).join("");

  const owing = state.consumers.filter(c => c.outstandingBalance > 0)
    .sort((a, b) => b.outstandingBalance - a.outstandingBalance).slice(0, 5);
  document.getElementById("dashDues").innerHTML = owing.length ? `
    <table><thead><tr><th>ID</th><th>Name</th><th>Category</th><th>Due</th></tr></thead><tbody>
      ${owing.map(c => `<tr><td class="mono">#${c.id}</td><td>${esc(c.name)}</td><td>${badge(c.category)}</td><td class="mono">${rs(c.outstandingBalance)}</td></tr>`).join("")}
    </tbody></table>` : `<p class="muted">No pending dues.</p>`;

  const allBills = state.consumers.flatMap(c => c.billHistory).sort((a, b) => b.billId - a.billId).slice(0, 5);
  document.getElementById("dashBills").innerHTML = allBills.length ? `
    <table><thead><tr><th>Bill</th><th>Consumer</th><th>Amount</th><th>Status</th></tr></thead><tbody>
      ${allBills.map(b => `<tr><td class="mono">#${b.billId}</td><td>${esc(b.consumerName)}</td><td class="mono">${rs(b.amount)}</td><td>${statusPill(b.paid)}</td></tr>`).join("")}
    </tbody></table>` : `<p class="muted">No bills generated yet.</p>`;
}

/* =============================================================================
   CONSUMERS — table, search, sort, register, edit, remove, detail
   ========================================================================== */
let consumerSortKey = "";

function renderConsumerTable() {
  const query = document.getElementById("consumerSearch").value.trim().toLowerCase();
  let list = state.consumers.slice();
  if (query) list = list.filter(c => c.name.toLowerCase().includes(query));

  if (consumerSortKey === "name") list.sort((a, b) => a.name.toLowerCase().localeCompare(b.name.toLowerCase()));
  else if (consumerSortKey === "id") list.sort((a, b) => a.id - b.id);
  else if (consumerSortKey === "balance") list.sort((a, b) => b.outstandingBalance - a.outstandingBalance);

  const tbody = document.querySelector("#consumerTable tbody");
  if (!list.length) {
    tbody.innerHTML = `<tr class="empty-row"><td colspan="7">No consumers match. Register one to get started.</td></tr>`;
    return;
  }
  tbody.innerHTML = list.map(c => `
    <tr>
      <td class="mono">#${c.id}</td>
      <td>${esc(c.name)}</td>
      <td>${badge(c.category)}</td>
      <td>${esc(c.address)}</td>
      <td class="mono">${c.meter.previousReading} / ${c.meter.currentReading}</td>
      <td class="mono">${rs(c.outstandingBalance)}</td>
      <td class="row-actions">
        <button class="btn btn-secondary" data-view-id="${c.id}">View</button>
        <button class="btn btn-secondary" data-edit-id="${c.id}">Edit</button>
        <button class="btn btn-danger" data-remove-id="${c.id}">Remove</button>
      </td>
    </tr>`).join("");
}

document.getElementById("consumerSearch").addEventListener("input", renderConsumerTable);
document.getElementById("sortSelect").addEventListener("change", e => { consumerSortKey = e.target.value; renderConsumerTable(); });

document.querySelector("#consumerTable tbody").addEventListener("click", e => {
  const view = e.target.closest("[data-view-id]");
  const edit = e.target.closest("[data-edit-id]");
  const remove = e.target.closest("[data-remove-id]");
  if (view) openDetail(Number(view.dataset.viewId));
  if (edit) openConsumerForm(Number(edit.dataset.editId));
  if (remove) removeConsumer(Number(remove.dataset.removeId));
});

function removeConsumer(id) {
  const c = findConsumer(id);
  if (!c) return;
  if (c.outstandingBalance > 0) {
    if (!confirm(`Consumer #${c.id} (${c.name}) has an outstanding balance of ${rs(c.outstandingBalance)}. Remove anyway?`)) return;
  } else if (!confirm(`Remove consumer #${c.id} (${c.name})?`)) return;
  state.consumers = state.consumers.filter(x => x.id !== id);
  saveState();
  renderConsumerTable();
  refreshAllSelects();
  toast(`Removed consumer #${id}.`);
}

/* ---- Register / Edit modal ---- */
const modalOverlay = document.getElementById("modalOverlay");
document.getElementById("openRegisterBtn").addEventListener("click", () => openConsumerForm(null));
document.getElementById("modalClose").addEventListener("click", closeConsumerForm);
modalOverlay.addEventListener("click", e => { if (e.target === modalOverlay) closeConsumerForm(); });

function openConsumerForm(id) {
  const form = document.getElementById("consumerForm");
  form.reset();
  document.getElementById("formConsumerId").value = id || "";
  const editing = id !== null;
  document.getElementById("modalTitle").textContent = editing ? "Update Consumer Details" : "Register New Consumer";
  document.getElementById("formSubmitBtn").textContent = editing ? "Save Changes" : "Register Consumer";
  document.getElementById("categoryField").classList.toggle("hidden", editing);
  document.getElementById("readingField").classList.toggle("hidden", editing);
  document.getElementById("formCategory").required = !editing;
  document.getElementById("formReading").required = !editing;

  if (editing) {
    const c = findConsumer(id);
    document.getElementById("formName").value = c.name;
    document.getElementById("formAddress").value = c.address;
  }
  modalOverlay.classList.remove("hidden");
  document.getElementById("formName").focus();
}
function closeConsumerForm() { modalOverlay.classList.add("hidden"); }

document.getElementById("consumerForm").addEventListener("submit", e => {
  e.preventDefault();
  const idVal = document.getElementById("formConsumerId").value;
  const name = document.getElementById("formName").value.trim();
  const address = document.getElementById("formAddress").value.trim();

  if (!/^[A-Za-z][A-Za-z .'-]{1,}$/.test(name)) { toast("That name looks invalid.", true); return; }
  if (!address) { toast("Address can't be blank.", true); return; }

  if (idVal) {
    const c = findConsumer(Number(idVal));
    c.name = name; c.address = address;
    toast(`Consumer #${c.id} updated.`);
  } else {
    const category = document.getElementById("formCategory").value;
    const reading = Math.max(0, parseInt(document.getElementById("formReading").value, 10) || 0);
    const c = makeConsumer(name, address, category, reading);
    state.consumers.push(c);
    toast(`Registered consumer #${c.id} (${category}).`);
  }
  saveState();
  closeConsumerForm();
  renderConsumerTable();
  refreshAllSelects();
});

/* ---- Detail modal ---- */
const detailOverlay = document.getElementById("detailOverlay");
document.getElementById("detailClose").addEventListener("click", () => detailOverlay.classList.add("hidden"));
detailOverlay.addEventListener("click", e => { if (e.target === detailOverlay) detailOverlay.classList.add("hidden"); });

function openDetail(id) {
  const c = findConsumer(id);
  if (!c) return;
  document.getElementById("detailTitle").textContent = `Consumer #${c.id} — ${c.name}`;
  const bills = c.billHistory.slice().reverse();
  document.getElementById("detailBody").innerHTML = `
    <div class="detail-grid">
      <div><div class="k">Category</div><div class="v">${badge(c.category)}</div></div>
      <div><div class="k">Outstanding</div><div class="v">${rs(c.outstandingBalance)}</div></div>
      <div><div class="k">Address</div><div class="v">${esc(c.address)}</div></div>
      <div><div class="k">Previous / Current read</div><div class="v">${c.meter.previousReading} / ${c.meter.currentReading}</div></div>
    </div>
    <h3 style="font-size:14px;">Bill history</h3>
    <div class="table-wrap">
      ${bills.length ? `<table><thead><tr><th>Bill</th><th>Date</th><th>Units</th><th>Amount</th><th>Paid</th><th>Status</th></tr></thead><tbody>
        ${bills.map(b => `<tr><td class="mono">#${b.billId}</td><td class="mono">${b.date}</td><td class="mono">${b.unitsConsumed}</td><td class="mono">${rs(b.amount)}</td><td class="mono">${rs(b.amountPaid)}</td><td>${statusPill(b.paid)}</td></tr>`).join("")}
      </tbody></table>` : `<p class="muted">No bills generated yet.</p>`}
    </div>`;
  detailOverlay.classList.remove("hidden");
}

/* =============================================================================
   METER READING
   ========================================================================== */
function renderMeterView() {
  populateConsumerSelect("meterConsumer");
  document.getElementById("meterInfo").classList.add("hidden");
  document.getElementById("meterForm").reset();
}
document.getElementById("meterConsumer").addEventListener("change", e => {
  const c = findConsumer(Number(e.target.value));
  const info = document.getElementById("meterInfo");
  if (!c) { info.classList.add("hidden"); return; }
  info.classList.remove("hidden");
  info.innerHTML = `Previous reading: <strong>${c.meter.previousReading}</strong> &nbsp;|&nbsp; Current reading: <strong>${c.meter.currentReading}</strong>`;
  document.getElementById("meterReading").min = c.meter.previousReading;
});
document.getElementById("meterForm").addEventListener("submit", e => {
  e.preventDefault();
  const id = Number(document.getElementById("meterConsumer").value);
  const c = findConsumer(id);
  if (!c) { toast("Select a consumer first.", true); return; }
  const reading = parseInt(document.getElementById("meterReading").value, 10);
  if (reading < c.meter.previousReading) {
    toast(`Current reading (${reading}) cannot be less than previous reading (${c.meter.previousReading}).`, true);
    return;
  }
  c.meter.currentReading = reading;
  saveState();
  toast(`Reading updated. Units so far this cycle: ${unitsConsumed(c)}.`);
  renderMeterView();
  document.getElementById("meterConsumer").value = id;
  document.getElementById("meterConsumer").dispatchEvent(new Event("change"));
});

/* =============================================================================
   GENERATE BILL
   ========================================================================== */
function renderBillingView() {
  populateConsumerSelect("billConsumer");
  document.getElementById("billInfo").classList.add("hidden");
  document.getElementById("billResult").classList.add("hidden");
  document.getElementById("billForm").reset();
  document.getElementById("billDate").value = todayStr();
}
document.getElementById("billConsumer").addEventListener("change", e => {
  const c = findConsumer(Number(e.target.value));
  const info = document.getElementById("billInfo");
  if (!c) { info.classList.add("hidden"); return; }
  const units = unitsConsumed(c);
  info.classList.remove("hidden");
  info.innerHTML = `Units consumed this cycle: <strong>${units}</strong> &nbsp;|&nbsp; Estimated amount: <strong>${rs(units > 0 ? Tariff[c.category](units) : 0)}</strong>`;
});
document.getElementById("billForm").addEventListener("submit", e => {
  e.preventDefault();
  const id = Number(document.getElementById("billConsumer").value);
  const c = findConsumer(id);
  if (!c) { toast("Select a consumer first.", true); return; }
  if (unitsConsumed(c) <= 0) {
    toast("No new consumption recorded since last bill — enter a meter reading first.", true);
    return;
  }
  const date = document.getElementById("billDate").value || todayStr();
  const bill = generateBillFor(c, date);
  saveState();
  const resultEl = document.getElementById("billResult");
  resultEl.classList.remove("hidden");
  resultEl.innerHTML = `
    <div class="notice success">Generated Bill #${bill.billId} for ${esc(c.name)} — ${rs(bill.amount)} (${bill.unitsConsumed} units)</div>
    <div class="btn-row">
      <button class="btn btn-secondary" id="viewReceiptAfterBill">View / Save Receipt</button>
    </div>`;
  document.getElementById("viewReceiptAfterBill").addEventListener("click", () => {
    switchView("receipts");
    document.getElementById("receiptConsumer").value = id;
    document.getElementById("receiptConsumer").dispatchEvent(new Event("change"));
    showReceipt(c, bill);
  });
  refreshAllSelects();
  toast(`Bill #${bill.billId} generated.`);
});

/* =============================================================================
   RECORD PAYMENT
   ========================================================================== */
function renderPaymentView() {
  populateConsumerSelect("paymentConsumer");
  document.getElementById("paymentInfo").classList.add("hidden");
  document.getElementById("paymentForm").reset();
}
document.getElementById("paymentConsumer").addEventListener("change", e => {
  const c = findConsumer(Number(e.target.value));
  const info = document.getElementById("paymentInfo");
  if (!c) { info.classList.add("hidden"); return; }
  info.classList.remove("hidden");
  info.innerHTML = `Outstanding balance: <strong>${rs(c.outstandingBalance)}</strong>`;
  document.getElementById("paymentAmount").max = c.outstandingBalance || undefined;
});
document.getElementById("paymentForm").addEventListener("submit", e => {
  e.preventDefault();
  const id = Number(document.getElementById("paymentConsumer").value);
  const c = findConsumer(id);
  if (!c) { toast("Select a consumer first.", true); return; }
  if (c.outstandingBalance <= 0) { toast("This consumer has no outstanding balance.", true); return; }
  const amount = parseFloat(document.getElementById("paymentAmount").value);
  if (!(amount > 0)) { toast("Payment amount must be positive.", true); return; }
  recordPaymentFor(c, amount);
  saveState();
  toast(`Payment recorded. New balance: ${rs(c.outstandingBalance)}.`);
  renderPaymentView();
  document.getElementById("paymentConsumer").value = id;
  document.getElementById("paymentConsumer").dispatchEvent(new Event("change"));
  refreshAllSelects();
});

/* =============================================================================
   RECEIPTS
   ========================================================================== */
function renderReceiptsView() {
  populateConsumerSelect("receiptConsumer");
  document.getElementById("receiptBillList").innerHTML = "";
  document.getElementById("receiptPreviewCard").classList.add("hidden");
}
document.getElementById("receiptConsumer").addEventListener("change", e => {
  const c = findConsumer(Number(e.target.value));
  const list = document.getElementById("receiptBillList");
  document.getElementById("receiptPreviewCard").classList.add("hidden");
  if (!c) { list.innerHTML = ""; return; }
  if (!c.billHistory.length) { list.innerHTML = `<p class="muted">This consumer has no bills yet.</p>`; return; }
  const bills = c.billHistory.slice().reverse();
  list.innerHTML = `<table><thead><tr><th>Bill</th><th>Date</th><th>Amount</th><th>Status</th><th></th></tr></thead><tbody>
    ${bills.map(b => `<tr><td class="mono">#${b.billId}</td><td class="mono">${b.date}</td><td class="mono">${rs(b.amount)}</td><td>${statusPill(b.paid)}</td>
      <td><button class="btn btn-secondary" data-receipt-bill="${b.billId}">Preview</button></td></tr>`).join("")}
  </tbody></table>`;
  list.querySelectorAll("[data-receipt-bill]").forEach(btn => {
    btn.addEventListener("click", () => {
      const bill = c.billHistory.find(b => b.billId === Number(btn.dataset.receiptBill));
      showReceipt(c, bill);
    });
  });
});

let activeReceipt = null;
function showReceipt(c, bill) {
  activeReceipt = { c, bill };
  const text = buildReceiptText(c, bill);
  document.getElementById("receiptPreview").textContent = text;
  document.getElementById("receiptPreviewCard").classList.remove("hidden");
  document.getElementById("receiptPreviewCard").scrollIntoView({ behavior: "smooth", block: "nearest" });
}
function buildReceiptText(c, b) {
  const line = "=========================================";
  const div = "-----------------------------------------";
  return [
    line, "     ELECTRICITY BILL PAYMENT RECEIPT", line,
    `Bill No.        : ${b.billId}`,
    `Date            : ${b.date}`,
    div,
    `Consumer ID     : ${c.id}`,
    `Consumer Name   : ${c.name}`,
    `Address         : ${c.address}`,
    `Category        : ${c.category}`,
    div,
    `Units Consumed  : ${b.unitsConsumed}`,
    `Bill Amount     : ${rs(b.amount)}`,
    `Amount Paid     : ${rs(b.amountPaid)}`,
    `Balance Due     : ${rs(b.amount - b.amountPaid)}`,
    `Status          : ${b.paid ? "PAID" : "DUE"}`,
    line, "     Thank you for using our service.", line
  ].join("\n");
}
document.getElementById("printReceiptBtn").addEventListener("click", () => {
  if (!activeReceipt) return;
  const w = window.open("", "_blank", "width=420,height=640");
  w.document.write(`<pre style="font-family:'JetBrains Mono',monospace;font-size:13px;white-space:pre-wrap;">${esc(document.getElementById("receiptPreview").textContent)}</pre>`);
  w.document.close();
  w.focus();
  w.print();
});
document.getElementById("downloadReceiptBtn").addEventListener("click", () => {
  if (!activeReceipt) return;
  const blob = new Blob([document.getElementById("receiptPreview").textContent], { type: "text/plain" });
  const url = URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = `receipt_${activeReceipt.bill.billId}.txt`;
  a.click();
  URL.revokeObjectURL(url);
});

/* =============================================================================
   REPORTS
   ========================================================================== */
let currentReportTab = "summary";
document.getElementById("reportTabs").addEventListener("click", e => {
  const btn = e.target.closest(".tab");
  if (!btn) return;
  document.querySelectorAll("#reportTabs .tab").forEach(t => t.classList.toggle("active", t === btn));
  currentReportTab = btn.dataset.report;
  renderReport(currentReportTab);
});

function renderReport(kind) {
  const body = document.getElementById("reportBody");
  if (kind === "summary") return renderSummaryReport(body);
  if (kind === "dues") return renderDuesReport(body);
  if (kind === "top") return renderTopReport(body);
}

function renderSummaryReport(body) {
  let residential = 0, commercial = 0, industrial = 0;
  let totalCollected = 0, totalDues = 0, totalBilled = 0, billsIssued = 0, unpaid = 0;
  for (const c of state.consumers) {
    if (c.category === "Residential") residential++;
    else if (c.category === "Commercial") commercial++;
    else industrial++;
    totalDues += c.outstandingBalance;
    for (const b of c.billHistory) {
      totalBilled += b.amount;
      totalCollected += b.amountPaid;
      billsIssued++;
      if (!b.paid) unpaid++;
    }
  }
  body.innerHTML = `
    <h3>Overall Summary</h3>
    <div class="detail-grid">
      <div><div class="k">Total consumers</div><div class="v">${state.consumers.length}</div></div>
      <div><div class="k">By category</div><div class="v">${residential} res · ${commercial} com · ${industrial} ind</div></div>
      <div><div class="k">Total bills issued</div><div class="v">${billsIssued}</div></div>
      <div><div class="k">Unpaid bills</div><div class="v">${unpaid}</div></div>
      <div><div class="k">Total amount billed</div><div class="v">${rs(totalBilled)}</div></div>
      <div><div class="k">Total revenue collected</div><div class="v">${rs(totalCollected)}</div></div>
      <div><div class="k">Total pending dues</div><div class="v">${rs(totalDues)}</div></div>
    </div>`;
}

function renderDuesReport(body) {
  const owing = state.consumers.filter(c => c.outstandingBalance > 0)
    .sort((a, b) => b.outstandingBalance - a.outstandingBalance);
  if (!owing.length) { body.innerHTML = `<h3>Pending Dues Report</h3><p class="muted">No consumers currently have pending dues.</p>`; return; }
  const total = owing.reduce((s, c) => s + c.outstandingBalance, 0);
  body.innerHTML = `
    <h3>Pending Dues Report</h3>
    <div class="table-wrap"><table><thead><tr><th>ID</th><th>Name</th><th>Category</th><th>Due</th></tr></thead><tbody>
      ${owing.map(c => `<tr><td class="mono">#${c.id}</td><td>${esc(c.name)}</td><td>${badge(c.category)}</td><td class="mono">${rs(c.outstandingBalance)}</td></tr>`).join("")}
    </tbody></table></div>
    <p class="muted" style="margin-top:10px">Total pending across ${owing.length} consumer(s): ${rs(total)}</p>`;
}

function renderTopReport(body) {
  if (!state.consumers.length) { body.innerHTML = `<h3>Top Consumers Report</h3><p class="muted">No consumers registered yet.</p>`; return; }
  let highestBill = null, highestBillConsumer = null;
  let topLifetimeConsumer = null, topLifetimeAmount = -1;
  for (const c of state.consumers) {
    let lifetime = 0;
    for (const b of c.billHistory) {
      lifetime += b.amount;
      if (!highestBill || b.amount > highestBill.amount) { highestBill = b; highestBillConsumer = c; }
    }
    if (lifetime > topLifetimeAmount) { topLifetimeAmount = lifetime; topLifetimeConsumer = c; }
  }
  body.innerHTML = `
    <h3>Top Consumers Report</h3>
    <div class="detail-grid">
      <div><div class="k">Highest single bill</div><div class="v">${highestBill ? `${rs(highestBill.amount)} — #${highestBillConsumer.id} ${esc(highestBillConsumer.name)}` : "(no bills yet)"}</div></div>
      <div><div class="k">Top lifetime billing</div><div class="v">${topLifetimeConsumer && topLifetimeAmount > 0 ? `${rs(topLifetimeAmount)} — #${topLifetimeConsumer.id} ${esc(topLifetimeConsumer.name)}` : "(no bills yet)"}</div></div>
    </div>`;
}

/* =============================================================================
   BACKUP & RESTORE
   ========================================================================== */
function renderBackupView() {
  const list = document.getElementById("backupList");
  if (!state.backups.length) { list.innerHTML = `<p class="muted">No backups yet.</p>`; return; }
  const rows = state.backups.slice().reverse();
  list.innerHTML = `<table><thead><tr><th>Created</th><th>Consumers</th><th>Bills</th><th></th></tr></thead><tbody>
    ${rows.map(b => `<tr><td class="mono">${b.label}</td><td class="mono">${b.snapshot.consumers.length}</td><td class="mono">${b.snapshot.consumers.reduce((s, c) => s + c.billHistory.length, 0)}</td>
      <td><button class="btn btn-danger" data-restore="${b.label}">Restore</button></td></tr>`).join("")}
  </tbody></table>`;
  list.querySelectorAll("[data-restore]").forEach(btn => {
    btn.addEventListener("click", () => restoreBackup(btn.dataset.restore));
  });
}
document.getElementById("backupBtn").addEventListener("click", () => {
  const label = new Date().toLocaleString();
  state.backups.push({ label, snapshot: JSON.parse(JSON.stringify({ consumers: state.consumers, consumerCount: state.consumerCount, billCounter: state.billCounter })) });
  saveState();
  renderBackupView();
  toast("Backup created.");
});
function restoreBackup(label) {
  const b = state.backups.find(x => x.label === label);
  if (!b) return;
  if (!confirm(`Restore data from backup "${label}"? This overwrites current data.`)) return;
  state.consumers = JSON.parse(JSON.stringify(b.snapshot.consumers));
  state.consumerCount = b.snapshot.consumerCount;
  state.billCounter = b.snapshot.billCounter;
  saveState();
  renderConsumerTable();
  refreshAllSelects();
  renderBackupView();
  toast(`Data reloaded from backup "${label}". ${state.consumers.length} consumer(s) in memory.`);
}
document.getElementById("exportBtn").addEventListener("click", () => {
  const blob = new Blob([JSON.stringify(state, null, 2)], { type: "application/json" });
  const url = URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = `ebs_export_${todayStr()}.json`;
  a.click();
  URL.revokeObjectURL(url);
});
document.getElementById("importFile").addEventListener("change", e => {
  const file = e.target.files[0];
  if (!file) return;
  const reader = new FileReader();
  reader.onload = () => {
    try {
      const imported = JSON.parse(reader.result);
      if (!imported.consumers) throw new Error("not a valid export file");
      if (!confirm("Import this file? It will overwrite all current data.")) return;
      state = imported;
      saveState();
      renderConsumerTable();
      refreshAllSelects();
      renderBackupView();
      renderDashboard();
      toast("Data imported.");
    } catch (err) {
      toast("Could not import that file: " + err.message, true);
    }
  };
  reader.readAsText(file);
  e.target.value = "";
});

/* =============================================================================
   INIT
   ========================================================================== */
function init() {
  state = loadState();
  saveState();
  document.getElementById("pwUser").value = state.admin.user;
  document.getElementById("billDate").value = todayStr();
  refreshAllSelects();
  ledSequence();
}
init();
