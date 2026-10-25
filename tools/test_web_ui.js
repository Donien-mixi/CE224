// Run with: node tools/test_web_ui.js
// Exercise embedded page scripts without a board or third-party dependencies.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

class Element {
  constructor() {
    this.children = []; this.dataset = {}; this.checked = false;
    this.disabled = false; this.value = '0'; this._text = '';
    this.classList = {toggle() {}};
  }
  set textContent(value) { this._text = value; this.children = []; }
  get textContent() { return this._text; }
  appendChild(child) { child.parent = this; this.children.push(child); }
  replaceChildren() { this.children = []; }
  remove() { this.parent.children = this.parent.children.filter(c => c !== this); }
  addEventListener() {}
  getContext() { return {clearRect() {}, beginPath() {}, moveTo() {}, lineTo() {}, stroke() {}}; }
}

function loadPage(filename) {
  const source = fs.readFileSync(path.join(__dirname, '../ESP32_S3_Gateway', filename), 'utf8');
  let script = source.match(/<script>([\s\S]*?)<\/script>/)[1];
  script = script.replace("selectMode('balance');connect();poll();setInterval(poll,200);", '');
  script = script.replace('describe();connect();poll();setInterval(poll,700);', '');
  const elements = new Map();
  const el = id => {
    if (!elements.has(id)) elements.set(id, new Element());
    return elements.get(id);
  };
  const context = vm.createContext({
    document: {hidden: false, getElementById: el, querySelector: el,
      querySelectorAll: () => [], createElement: () => new Element(), addEventListener() {}},
    window: {addEventListener() {}}, setInterval() {}, setTimeout() {},
    sessionStorage: {getItem: () => 'test-owner'},
    fetch: async () => { throw Error('offline'); }, AbortSignal, URLSearchParams,
  });
  vm.runInContext(script, context);
  return {context, el, run: code => vm.runInContext(code, context)};
}

async function dashboardTests() {
  const {context: c, el, run} = loadPage('dashboard_html.h');
  run(`
    var sample=Array(38).fill(0);sample[4]=3;sample[27]=12;
    sample[17]=12.34;sample[19]=sample[20]=0.4;
    var meta=Array(20).fill('0');meta[0]='$BINFO';meta[1]='BAL-20261018-R12-SPEED2-8DIR';
    meta[7]='100';meta[17]='12';meta[19]='60';
    var packet={seen:true,age_ms:0,row:sample.join(','),meta:meta.join(','),samples:1,capacity:10};
  `);
  const restore = async () => {
    c.fetch = async () => ({ok: true, json: async () => c.packet});
    await c.poll();
    assert.equal(c.fresh(), true);
    assert.equal(el('pitch').textContent, '12.34');
    assert.match(el('status').textContent, /ĐANG CHẠY/);
  };
  const assertUnavailable = () => {
    assert.equal(c.fresh(), false);
    assert.equal(el('pitch').textContent, '—');
    assert.equal(el('speed').textContent, '—');
    assert.equal(el('arm').disabled, true);
    assert.equal(el('.connection-details').open, true);
    assert.doesNotMatch(el('status').textContent, /ĐANG CHẠY/);
    assert.match(el('drive-status').textContent, /Chưa có dữ liệu mới/);
  };
  await restore();
  c.fetch = async () => { throw Error('offline'); };
  await c.poll();
  assertUnavailable();
  await restore();
  run('lastFresh=Date.now()-600;updateButtons();');
  assertUnavailable();
  await restore();
  run("packet.row='invalid';");
  await c.poll();
  assertUnavailable();
  run("packet.row=sample.join(',');packet.seen=false;");
  await c.poll();
  assertUnavailable();
  run('packet.seen=true;sample[4]=0;packet.row=sample.join(",");wsOK=true;');
  await restoreStopped();
  async function restoreStopped() {
    await c.poll();
    assert.equal(c.fresh(), true);
    assert.match(el('arm-reason').textContent, /Đánh dấu xác nhận/);
    el('floor').checked = true;
    c.updateButtons();
    assert.equal(el('arm').disabled, false);
    c.selectMode('bench');
    assert.equal(el('arm').disabled, true);
    assert.match(el('arm-reason').textContent, /Đang chọn thử bánh/);
  }
}

function logTests() {
  const {context: c, el, run} = loadPage('wheel_tests_html.h');
  run(`ms={jobs:[{id:1,plan:61,sealed:true,phase:2,samples:10}],active:0,available:true};render();`);
  const first = el('logs').children[0];
  const download = first.children[1];
  run(`ms.jobs=[{id:1,plan:61,sealed:true,phase:2,samples:20}];render();`);
  assert.equal(el('logs').children[0], first);
  assert.equal(first.children[1], download);
  assert.match(first.children[0].textContent, /20 mẫu/);
  assert.equal(c.logRows.get(1).job.samples, 20);
  run('downloadBusy=true;render();');
  assert.equal(download.disabled, true);
  run('downloadBusy=false;render();');
  assert.equal(download.disabled, false);
  run('ms.jobs=[];render();');
  assert.equal(el('logs').children.length, 0);
  assert.equal(c.logRows.size, 0);
}

(async () => {
  await dashboardTests();
  logTests();
  console.log('PASS: telemetry failures, expiry, recovery, arm guidance, and stable log controls');
})().catch(error => { console.error(error); process.exitCode = 1; });
