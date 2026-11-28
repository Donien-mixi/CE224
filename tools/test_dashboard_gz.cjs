// Kiểm tra dashboard_html_gz.h khớp với dashboard_html.h (chống quên sinh lại).
const fs=require('fs'),zlib=require('zlib'),assert=require('assert');
const t=fs.readFileSync('ESP32_S3_Gateway/dashboard_html.h','utf8');
const m=t.match(/R"dash\(([\s\S]*)\)dash"/);
assert.ok(m,'DASHBOARD_HTML not found');
const gz=zlib.gzipSync(Buffer.from(m[1],'utf8'),{level:9});
const h=fs.readFileSync('ESP32_S3_Gateway/dashboard_html_gz.h','utf8');
const len=Number(/DASHBOARD_GZ_LEN=(\d+)/.exec(h)[1]);
const bytes=[...h.matchAll(/0x([0-9a-f]{2})/g)].map(x=>parseInt(x[1],16));
assert.strictEqual(len,gz.length,'length mismatch: chạy node tools/gen_dashboard_gz.cjs');
assert.strictEqual(bytes.length,gz.length,'byte count mismatch');
assert.ok(Buffer.from(bytes).equals(gz),'gzip bytes mismatch: chạy node tools/gen_dashboard_gz.cjs');
console.log(`PASS dashboard gzip: ${m[1].length} B HTML -> ${gz.length} B gzip (${(100-gz.length*100/m[1].length).toFixed(1)}% nhỏ hơn), header khớp.`);
