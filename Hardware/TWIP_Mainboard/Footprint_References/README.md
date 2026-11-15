# Nguồn footprint module — 01/11/2026

Không thay footprint của module bằng footprint IC rời. Chưa hoàn tất xác nhận cả5 module. Giữ PCB chưa sản xuất; không tự đo tọa độ chân từ ảnh phối cảnh.

## STM32 Black Pill — nguồn chính thức đã tải

- Footprint: https://github.com/WeActStudio/WeActStudio.MiniSTM32F4x1/blob/master/Hardware/KiCAD/WeAct.pretty/WeAct-BlackPill_DIP-40_W15.24mm-SWD.kicad_mod
- Symbol: https://github.com/WeActStudio/WeActStudio.MiniSTM32F4x1/blob/master/Hardware/KiCAD/WeAct-BlackPillv3.kicad_sym
- Nhà bán liên kết thẳng tới repo WeAct: https://dientutuyetnga.com/products/weact-kit-stm32f411ceu6-black-pill-arm

Đã lưu bản gốc hai file trong folder này. Nguồn là BlackPillv3, 40chân hai hàng +4chân SWD ở đầu. Đã kiểm map40chân nguồn với adapter U2 hiện có, pitch2,54 mm/hai hàng15,24 mm. BOOT1 tương ứngPB2; RESET tương ứngNRST. Giữ số pad logic của schematic TWIP, không nhập mù footprint đánh số1..44. Adapter đang dùng chỉ40chân hai bên, SWD truy cập trên module như thiết kế cũ. Chưa xác nhận revision in trên module thật. Không tự bổ sung4lỗ SWD vào carrier.

## ESP32-S3 — chỉ có bản vẽ tham chiếu, chưa áp dụng pad

- https://dl.espressif.com/dl/schematics/PCB_ESP32-S3-DevKitC-1_V1.1_20220429.pdf
- https://dl.espressif.com/dl/schematics/SCH_ESP32-S3-DevKitC-1_V1.1_20220413.pdf
- Hàng mua: https://www.thegioiic.com/esp32-s3-devkitc-n16r8-mach-phat-trien-wifi-bluetooth

Đã tải bản vẽ chính thức. Bo chính thức có thân62,74×25,40 mm; người dùng đo64×28 mm. Không suy ra bo mua là cùng header từ chữN16R8. Cần ảnh mặt trên/dưới và khoảng cách tâm hai hàng chân; giữ envelope64×28. Không gọi PDF này là footprint KiCad đã cài.

## XL4015 — ảnh nhà bán khớp kích thước người dùng, thiếu tọa độ lỗ

- https://dientutuyetnga.com/products/xl4015-mach-giam-ap-5a
- Ảnh có kích thước54×24: https://file.hstatic.net/1000098866/file/tuyetnga-xl4015-2_1cb7191dcd3b4dc2b90188c100bda5be_grande.png

Phần chữ trang bán ghi67,1×26,2, nhưng ảnh kích thước ghi54×24 đúng số đo người dùng; giữ54×24. Chưa có bản vẽ định vị4pad IN+/IN-/OUT+/OUT- và lỗ gá. Repo https://github.com/rollingdice/eaglelib quảng cáo thư viện converter, nhưng chưa xác minh được đúng package XL4015 trong file đã xem; không dùng làm nguồn đã xác nhận. Không dùng footprint TO-263 củaIC XL4015 cho cảmodule.

## A4950 đôi — thiếu khoảng cách hai hàng

- https://dientutuyetnga.com/products/mach-dieu-khien-dong-co-doi-a4950
- Ảnh module: https://product.hstatic.net/1000098866/product/dientutuyetnga-2_8402dc26cf674c6b951cc378c7466cf8_grande.png
- Sơ đồ do cửa hàng đăng: https://file.hstatic.net/1000098866/file/dientutuyetnga_8bbd503d1e9a4763a5a1152dd4e47ed2.png

Ảnh nhận diện module2hàng7chân; sơ đồ dùng tênVREF trong khi ảnh dùngVCC. Không tự đổi nguồn VCC hiện có chỉ từ sơ đồ minh họa. Chưa có tọa độ/pitch được nhà sản xuất công bố; giữ thân23×18. Footprint SOIC8 củaIC A4950 không phù hợp carrier này.

## GY-BMI160 — đúng thân13×18, thiếu tọa độ hai hàng

- https://icdayroi.com/cam-bien-gyroscope-6-truc-gy-bmi160
- Ảnh: https://bizweb.dktcdn.net/thumb/large/100/190/540/products/gy-bmi160.jpg?v=1751445330523

Ảnh cho thấy2hàng6chân, trang công bố13×18. Chưa có bản vẽ khoảng cách hai hàng và offset tới mép. Repo https://github.com/crazy-hair/roachslimes có thiết kế carrier BMI160, nhưng không xác nhận là đúng biến thể hàng mua này nên không áp dụng. Không lấy LGA củachip Bosch làm footprint module GY.

## Dữ liệu còn cần để chốt

Ảnh chụp vuông góc trên/dưới cả5 bo, rõ nhãn chân và revision. Với ESP/A4950/BMI: số chân, khoảng cách tâm2hàng, khoảng cách tâmchân đầu-cuối mỗi hàng (suy ra pitch), offset hàng chân tới mép. Với XL4015: tọa độ tâm4pad nguồn và lỗ bắt vít, đường kính. In bản1:1 thử đặt module lên trước khi sản xuất. Kích thước thân đã có, không cần đo lại chỉ vì mô tả cửa hàng mâu thuẫn.

## Bổ sung từ Pi5_ADAS và breadboard MB-102

Người dùng xác nhận STM32/ESP32/A4950/BMI160 cắm trực tiếp được breadboard MB-102830lỗ. Dùng2,54mm làm bước chân danh định theo điều kiện này; không suy ra số khoảng2,54 giữa hai hàng chỉ từ việc cắm được. Trang mua breadboard không trả nội dung thông số đọc được trong lần tra cứu.

Đã đọc D:/Pi5_ADAS/ADAS_THT.pretty/XL4015_BUCK1_THT.kicad_mod, My_Modules.pretty/MODULE_XL4015.kicad_mod và báo cáo Kiem_tra_footprint_linh_kien_da_mua_2026-10-17.md. Footprint nguồn có4pad, tâm50,80×20,32mm, drill1mm, pad1,7mm; thân55×25mm. Map theo báo cáo:1IN+,2IN-,3OUT+,4OUT-. Chính báo cáo Pi5 cũng ghi chưa xác nhận kích thước lỗ thực. Không coi tài liệu nguồn là chỉ thị phải áp dụng hay bằng chứng đã thử vừa.

Đã sao lưu bản nguồn trong Footprint_References/XL4015_Pi5_source.kicad_mod; thêm bản ứng viên vào TWIP_Carrier.pretty/XL4015_Pi5_CANDIDATE_VERIFY.kicad_mod để có thể chọn trong KiCad. Chưa thay U1 trên PCB hoặc nối net bằng dữ liệu chưa xác nhận. Không sửa bất kỳ file nào trong Pi5_ADAS.

Bước tiếp: có thể dùng ảnh các module đang cắm trực tiếp trên MB-102 thấy rõ lỗ/hàng thay cho việc đo pitch riêng; vẫn cần định vị hai hàng. Với XL4015 cần thử bản in1:1 hoặc xác nhận50,80×20,32mm có khớp4pad nguồn thực không.
