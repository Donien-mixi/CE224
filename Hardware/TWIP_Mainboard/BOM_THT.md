> Bản PCB ngày 06/11/2026 dùng một mặt đồng B.Cu, 31 đoạn dây cầu trong 24 nhóm. Tâm thân BMI160 tại (130,00; 81,65) mm. Xem [SINGLE_LAYER.md](SINGLE_LAYER.md) và [bảng dây cầu](SINGLE_LAYER_WIRES.csv) cho bản lắp hiện hành. Các tọa độ bố trí và số dây của bản trước bên dưới là dữ liệu lịch sử.

# Danh sách tụ — bản toàn bộ chân cắm

Bản hiện hành thay thế danh sách0805 trước đây. Toàn bộ linh kiện rời lắp trên carrier dùng chân xuyên lỗ; không thay linh kiện dán có sẵn trong các module.

| Vị trí | SL | Giá trị / điện áp tối thiểu | Kiểu cần mua |
|---|---:|---|---|
| C1 |1|1000µF /16V|Tụ hóa radial Ø8×16 mm, chân 5 mm theo link Thế Giới IC; ưu tiên 105°C, cần xác nhận ESR/ripple |
| C2 |1|470µF /35V|Tụ hóa radial low-impedance105°C, Ø10mm, chân5mm, cao20mm; gợi ý NichiconUHE1V471MPD đúngcase |
| C3 |1|220nF /50V|Tụ gốm MLCC radial X7R, không phân cực, chân5mm |
| C4,C6 |2|10µF /16V|Tụ hóa phân cực chân cắm OEM IC4469, Ø4×7 mm, bước chân 2 mm, 105°C; pad 1 (+) vào 5V, pad 2 (−) vào GND |
| C5,C7,C9 |3|100nF /50V|Tụ gốm MLCC radial X7R, không phân cực, chân5mm |
| C8 |1|1µF /50V|Tụ gốm MLCC radial X7R, không phân cực, chân5mm |

Footprint C3/C5/C7/C8/C9 dành cho thân tối đa6×3,5mm nhìn từ trên xuống, chân tối đaØ0,6mm, bước5mm; lỗPCBØ0,9mm/padØ1,8mm. Chiều cao thân nên≤6mm để thuận bố trí, phải kiểm khoảng trống tầngxe thực. Không mua tụ0805; C4/C6 hiện đã đổi sang footprint tụ hóa phân cực D4/P2; không cắm ngược cực. Tụ gốm chân 2,5 hoặc 2,54 mm chưa khớp footprint MLCC 5 mm hiện hành. C4/C6 là ngoại lệ có bước chân 2 mm đã thiết kế riêng.

Tham khảo phương án MLCC cũ, **không phải loại C4/C6 đang chọn**: **TDK FG26X7R1E106KRT06**,25V,X7R, thân dài tối đa5,5mm/dày3,5mm/cao6mm, khoảng chân5mm danh định. Hãng hỗ trợ hàn mỏ. Nguồn: https://product.tdk.com/en/search/capacitor/ceramic/lead-mlcc/info?part_no=FG26X7R1E106KRT06 . Phải kiểm mã hàng/định dạng chân khi đặt mua; 10µF danh định chưa chứng minh điện dung hiệu dụng ở5V.

Nguồn hãng của phương án Nichicon trước đây (C1 D10 không còn là đường bao đang chọn, không tự thay vào bản D8): https://www.nichicon.co.jp/english/series_items/catalog_pdf/e-uhe.pdf và https://www.nichicon.com/en-us/part/uhe1v471mpd/5835/ . Chọn chân thẳng/case khớpPCB, không tự đổi sang các biến thể thân lớn hơn. Chưa mua linh kiện hoặc xác nhận tồn kho người dùng trong lượt này.

Ngày sửa01/11/2026: đã thay C3–C9 trong PCB và schematic, đi dây lại toàn bo, kiểm148pad đềuPTH/0padSMD; netlist gốc không đổi. ERC0/DRC0/unconnected0/parity0. Kiểm cơ khí/module/anten/khoảng gá và thử nguồn tải vẫn cần trước khi sản xuất.

## Link sản phẩm để xem xét mua

Cập nhật ngày 01/11/2026. Các link dưới đây là ứng viên mua hàng, chưa phải danh sách đã xác nhận lắp vừa toàn bộ PCB. Giá và tồn kho có thể thay đổi. Số lượng là số lắp trên một bo; shop có thể yêu cầu mua theo số lượng tối thiểu.

| Vị trí | SL | Giá trị cần dùng | Link sản phẩm | Trạng thái đối chiếu |
|---|---:|---|---|---|
| C1 | 1 | 1000 µF / 16 V | [Thế Giới IC — tụ hóa xuyên lỗ 8×16 mm](https://www.thegioiic.com/tu-hoa-1000uf-16v-8x16mm-xuyen-lo) | Đúng trị số; trang shop ghi bước chân 5 mm. PCB đã dùng đường bao Ø8 mm/P5 cho mẫu này. Cần xác nhận ESR/dòng ripple và loại 105°C trước khi chốt thay mã Nichicon đề xuất. |
| C2 | 1 | 470 µF / 35 V | [Thế Giới IC — tụ hóa xuyên lỗ 10×17 mm](https://www.thegioiic.com/tu-hoa-470uf-35v-10x17mm-xuyen-lo); [IC Đầy Rồi — 470 µF / 35 V](https://icdayroi.com/tu-hoa-470uf35v) | Mẫu Thế Giới IC ghi bước chân 5 mm; thông tin chiều cao trên trang có chỗ 17 mm, có chỗ 20 mm, cần xác nhận hàng thực. Chưa xác nhận ESR/dòng ripple. Mẫu IC Đầy Rồi cần xác nhận thêm bước chân và nhiệt độ định mức. |
| C3 | 1 | 220 nF / 50 V, mã 224 | [Thế Giới IC — tụ gốm multilayer 220 nF / 50 V](https://www.thegioiic.com/tu-gom-multilayer-220nf-50v) | Đúng trị số; chưa xác nhận X7R, bước chân 5 mm và kích thước thân. |
| C4, C6 | 2 | 10 µF / ≥16 V | [Thế Giới IC — OEM IC4469, tụ hóa xuyên lỗ 10 µF / 16 V, 4×7 mm](https://www.thegioiic.com/tu-hoa-10uf-16v-4x7mm-xuyen-lo) | **Đã áp dụng vào PCB/schematic.** Thông số shop: ±20%, −40…105°C, thân Ø4×7 mm, bước chân **2 mm**. Giữ nguyên điện dung 10 µF; tụ có phân cực. Đã đổi ký hiệu sang tụ phân cực, footprint D4/P2, pad 1 (+) nối +5V và pad 2 (−) nối GND. Cần kiểm nguồn thực tế khi lắp. ESR/dòng ripple chưa được công bố đủ để xác nhận. |
| C5, C7, C9 | 3 | 100 nF / ≥16 V, mã 104 | [Thế Giới IC — tụ gốm multilayer 100 nF / 50 V](https://www.thegioiic.com/tu-gom-multilayer-100nf-0-1uf-50v) | Đúng trị số; chưa xác nhận X7R, bước chân 5 mm và kích thước thân. |
| C8 | 1 | 1 µF / ≥16 V, mã 105 | [Thế Giới IC — tụ gốm multilayer 1 µF / 50 V](https://www.thegioiic.com/tu-gom-multilayer-1uf-50v); [Tuyết Nga — tụ không cực 105](https://dientutuyetnga.com/products/tu-khong-cuc-105-pi) | Schematic/PCB ghi 1 µF / 50 V theo mẫu Thế Giới IC; chưa xác nhận X7R và cơ khí. Mẫu Tuyết Nga còn cần xác nhận cả điện áp và loại tụ. |

### Các lựa chọn khác cho C4/C6

- **Cập nhật thay hàng ngày 01/11/2026:** người dùng báo [Panasonic ECEA1CKA100](https://www.thegioiic.com/tu-hoa-10uf-16v-4x7mm-xuyen-lo-panasonic-ecea1cka100) hết hàng, nên không còn dùng làm link mua chính. Chọn ứng viên OEM IC4469 ở bảng trên để giữ 10 µF, không cần tăng điện dung chỉ vì thiếu hàng. Kết quả tìm kiếm trang sản phẩm hiển thị còn 530 chiếc, tối thiểu 10 chiếc/bội số 5; bản thu thập cách thời điểm tra cứu khoảng 3 tuần, **không phải xác nhận tồn kho trực tiếp lúc đặt hàng**. Kiểm lại số lượng và giá trên shop trước khi mua. Chưa tìm thấy tụ gốm chân cắm 10 µF ở Thế Giới IC có đủ thông số để chốt thay.
- [Thế Giới IC — JWCO, tụ hóa xuyên lỗ 22 µF / 16 V, 4×8 mm](https://www.thegioiic.com/tu-hoa-22uf-16v-4x8mm-xuyen-lo): ứng viên dự phòng nếu mẫu 10 µF cũng hết; danh mục shop được thu thập gần đây hiển thị có hàng. Đây là thay đổi điện dung, chưa áp dụng vào CAD; cần xác nhận bước chân/thông số đầy đủ, sửa footprint và thử đáp ứng nguồn. Ưu tiên mẫu 10 µF ở bảng trên nếu còn hàng.
- [Tuyết Nga — tụ không cực 106](https://dientutuyetnga.com/products/tu-khong-cuc-106-pi): mã 106 thường tương ứng 10 µF, nhưng trang chưa đủ thông tin để chốt điện áp, vật liệu, kích thước và bước chân. Chưa xác nhận là mẫu MLCC phù hợp PCB.
- [Thế Giới IC — Vishay MAL202136479E3, tụ hóa xuyên lỗ 47 µF / 25 V](https://www.thegioiic.com/tu-hoa-47uf-25v-4-5x10mm-xuyen-lo-vishay-mal202136479e3): phương án tăng điện dung và đổi sang tụ phân cực; chưa áp dụng vào thiết kế. Cần đối chiếu datasheet, cơ khí và đáp ứng nguồn.

C4 nằm giữa +5V/GND sát nguồn vào bo ESP32; C6 nằm giữa +5V/GND sát nguồn vào Black Pill. Đây là tụ lọc nguồn bổ sung trên carrier. Đã chuyển C4/C6 sang tụ hóa 10 µF, giữ C5/C7 là tụ gốm 100 nF, đánh dấu cực **+ vào +5V, − vào GND** và đi dây lại theo footprint mới. Không coi tụ hóa là tương đương tụ gốm chỉ vì cùng điện dung; ESR và đặc tính theo tần số khác nhau. Không ưu tiên giảm xuống 1–4,7 µF khi chưa đo đáp ứng tải. Tăng điện dung cũng cần kiểm dòng nạp và khả năng khởi động nguồn.

Tham khảo nguyên tắc: [TI — đặc tính ESR và trở kháng của các loại tụ lọc](https://www.ti.com/document-viewer/lit/html/SSZTBI6). [Espressif — hướng dẫn nguồn ESP32-S3](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32s3/schematic-checklist.html) nói về nguồn chip 3,3 V; không dùng hướng dẫn đó để coi tụ ở đầu vào 5 V của devkit là cùng một vị trí điện.

Cập nhật CAD ngày 01/11/2026: C1 dùng D8/P5; C4/C6 dùng tụ phân cực D4/P2, 10 µF/16 V theo OEM IC4469; C5/C7/C9 ghi 100 nF/50 V, C8 ghi 1 µF/50 V. C2 giữ D10/P5. C3/C5/C7/C8/C9 giữ footprint gốm P5: shop chưa xác nhận đủ kích thước nên chưa chốt lắp vừa các mẫu đó. Các tụ 22/47 µF chỉ là phương án dự phòng, không phải cấu hình CAD hiện hành. Xem CHECK_drc_final.json và CHECK_erc_final.json cho kết quả kiểm tra bản mới. Chưa phê duyệt sản xuất hoặc xác nhận đáp ứng nguồn bằng thử tải.

## Kích thước PCB và lỗ gá

PCB hiện hành **160 × 65 mm**. Đã thêm 4 lỗ M3 **NPTH Ø3,2 mm**, đã đặt theo tọa độ người dùng: cách cạnh ngắn 7,5 mm, cạnh dài 2,5 mm; dự trữ vùng không đồng Ø8 mm. H1–H4 là lỗ cơ khí, không tăng số lượng tụ hay linh kiện điện. Xem [MOUNTING_HOLES.md](MOUNTING_HOLES.md) để biết tọa độ và các số đo cần chốt; chưa chọn mua cụ thể bộ vít/vòng đệm/cọc đỡ.

Phân phối nguồn đã tích hợp trên PCB, không cần WAGO khi lắp carrier. Pin vào J1, XL4015 và các module nhận nguồn qua chân cắm. Xem [POWER_DISTRIBUTION.md](POWER_DISTRIBUTION.md); chưa chọn mã đầu nối J1 theo dòng tải thực.
