// Sinh dashboard_html_gz.h từ dashboard_html.h (gzip mức 9).
// Chạy: node tools/gen_dashboard_gz.cjs  (bắt buộc sau mỗi lần sửa HTML)
const fs=require('fs'),zlib=require('zlib');
const src='ESP32_S3_Gateway/dashboard_html.h';
const out='ESP32_S3_Gateway/dashboard_html_gz.h';
const t=fs.readFileSync(src,'utf8');
const m=t.match(/R"dash\(([\s\S]*)\)dash"/);
if(!m)throw new Error('DASHBOARD_HTML raw string not found');
const html=Buffer.from(m[1],'utf8');
const gz=zlib.gzipSync(html,{level:9});
let s='#pragma once\n#include <pgmspace.h>\n';
s+='/* AUTO-GENERATED từ dashboard_html.h — KHÔNG sửa tay.\n';
s+=' * Sinh lại: node tools/gen_dashboard_gz.cjs (sau mỗi lần sửa HTML). */\n';
s+=`static const unsigned int DASHBOARD_GZ_LEN=${gz.length};\n`;
s+='static const unsigned char DASHBOARD_GZ[] PROGMEM={\n';
for(let i=0;i<gz.length;i+=16){
  s+='  '+Array.from(gz.slice(i,i+16)).map(b=>'0x'+b.toString(16).padStart(2,'0')).join(',')+',\n';
}
s+='};\n';
fs.writeFileSync(out,s);
console.log('dashboard_html.h:',html.length,'B ->',gz.length,'B gzip,',(100-gz.length*100/html.length).toFixed(1)+'% giảm');
