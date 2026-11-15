> Bản PCB ngày 06/11/2026 dùng một mặt đồng B.Cu, 31 đoạn dây cầu trong 24 nhóm. Tâm thân BMI160 tại (130,00; 81,65) mm. Xem [SINGLE_LAYER.md](SINGLE_LAYER.md) và [bảng dây cầu](SINGLE_LAYER_WIRES.csv) cho bản lắp hiện hành. Các tọa độ bố trí và số dây của bản trước bên dưới là dữ liệu lịch sử.

> KÍCH THƯỚC MODULE HIỆN HÀNH: xem MODULE_DIMENSIONS.md. Đã cập nhật theo mép chân mới02/11; số57/64 ESP,18×13 BMI thẳng cột,23×18 driver và XL51×21 trong lịch sử đã được thay thế một phần.

> STM32: đường bao thân theo mép1mm/3mm mới, chiều dài suy ra52,26mm; giữ nguyên lỗ và hàng chân. Xem cập nhật cuối tài liệu.

> HIỆN HÀNH: anten ESP32 nhô ngoài cạnh phải7mm, USB hướng trái chừa20mm; BMI về vùng giữa cũ. Phương án anten vào trong đã thay thế. Xem mục cuối tài liệu.

> BỐ TRÍ HIỆN HÀNH02/11: ESP USB-C ở cạnh phải, C6 ở dưới STM; xem mục lối cắm cáp cuối tài liệu. Các tọa độ/độ dài đường dây bản cũ đã thay đổi.

> KIỂM TRA HỆ THỐNG: chưa chốt phân vùng nhiễu. Encoder phải còn sát track motor và nhánh5V STM còn dài; mô hình Scilab hiện không kiểm chứng PCB. Xem VALIDATION_PLAN.md trước khi sản xuất.

> NGUỒN HIỆN HÀNH: phân nhánh trên PCB thay WAGO; xem POWER_DISTRIBUTION.md. Các mô tả đường nguồn trước đó là lịch sử.

> ESP32 HIỆN HÀNH: thân57 mm + lưỡi anten7 mm = tổng64 mm. Xem mục cập nhật cuối tài liệu; các ghi chú cũ coi57 mm là toàn bộ module đã được thay thế.

> CẬP NHẬT LỖ GÁ: tâm H1–H4 đã theo tọa độ người dùng (cm), không còn vị trí tạm cách mép 4,5 mm. Xem MOUNTING_HOLES.md. Hướng tiến = hàng 7 chân BMI160, hướng lùi = hàng 5 chân. Các tọa độ/lỗ tạm bên dưới là lịch sử.

> Kích thước hiện hành: **160 × 65 mm**. Đã thêm H1–H4 lỗ M3 NPTH Ø3,2 mm ở tọa độ tạm; xem MOUNTING_HOLES.md. Có 22 footprint: 18 linh kiện/đầu nối điện và 4 lỗ cơ khí. Các số lượng 18/148 PTH trong ghi chép trước đây chưa tính 4 lỗ NPTH.

> CẬP NHẬT HIỆN HÀNH 01/11/2026: toàn bộ carrier dùng THT. C1 D8/P5; C2 D10/P5; C4/C6 là tụ hóa phân cực 10 µF/16 V D4/P2 (OEM IC4469). C3/C5/C7/C8/C9 là tụ gốm radial P5, kích thước hàng mua còn cần xác nhận. Xem BOM_THT.md và các CHECK_* cho bản kiểm tra mới. Các mô tả cũ bên dưới không dùng để mua linh kiện.

# PCB đã đi dây — kiểm tra 01/11/2026

Trạng thái: đã nối toàn bộ net của sơ đồ hiện tại; chưa phê duyệt sản xuất. Đọc tài liệu này trước các trạng thái lịch sử trong PCB_DRAFT.md. Không thay đổi firmware.

## Đối chiếu sơ đồ và hướng dẫn

Đọc Huong_Dan_Di_Day_Toan_Bo_He_Thong.md, kiểm từng pad đang dùng với netlist mới xuất từ schematic. Giữ VBAT→VM/IN+, XL4015→5V các bo/VCC driver, STM32 3V3→BMI/CS/encoder, BMI SA0→GND; không cấp VIN BMI. UART PB6→GPIO18/PB7←GPIO17, PWM PA8..PA11, encoder và INT1→PB2 giữ đúng hướng dẫn. Hai chân VIN/INT2 không dùng được gán net no-connect cho nhất quán, không đi dây. OCS/SCX/SDX BMI giữ bỏ trống. Hai chân VM/VCC driver cùng rail tương ứng; hai GND cùng mốc, không cách ly.

Gán18 footprint vào schematic, sửa đường liên kết root/symbol UUID trên PCB. Chỉ sửa association/metadata; không đổi topology. Kiểm tra parity phát hiện và đã sửa value U2 khác schematic và hai net no-connect. Các dấu nguồn không được thay thế bằng tên chữ trang trí.

## Thiết kế đường đồng

- PCB160×65mm,2lớp. Netclass tín hiệu mục tiêu0,25mm; logic5V/3V3 mục tiêu0,7mm; VBAT và bốn đầu motor mục tiêu1,5mm. Có đoạn thu hẹp khi thoát pad: motor/VBAT có đoạn 1,125 mm; logic 0,7 mm trong bản cập nhật tụ; không tuyên bố mọi đoạn đạt1,5mm. Track nhỏ nhất sau kiểm tra0,2mm. Các đoạn nguồn/motor dài không dùng bề rộng tín hiệu.
- Route được xử lý bằng freerouting2.5.0 chạy local với các lớp độ rộng riêng, nhập session về KiCad; sau đó chỉnh lỗi mép/độ rộng, tăng via logic lên0,8/0,4mm, nối đặc GND driver với plane và đổ lại đồng. Không coi autorouter là bằng chứng về EMI/nhiệt.
- Bản cập nhật tụ: VBAT và ba nhánh motor không có via; MOTOR_L_M1 có 2 via Ø1 mm/lỗ0,5 mm, cần kiểm dòng/nhiệt thực tế. Tín hiệu/vài nhánh logic có via. Hai mặt đổGND, clearance0,25mm; thermal các pad thông thường có spoke0,5mm, GND driver nối đặc. Đường hồi motor có đường rộng/plane; chưa đo ground bounce và chưa chứng minh dòng hồi không đi qua vùng cảm biến bằng mô phỏng trường điện từ.
- C1 cạnh đầu raXL4015; C2/C3 phíaVM driver; C4/C5 đi theo nguồnESP; C6/C7 sát nguồnSTM; C8/C9 sát nguồnBMI. Giữ giá trịtụ. C1 dùng D8/P5/H16, C2 giữ D10/P5/H≤20. C4/C6 đổi sang tụ hóa D4/P2/H7 và bố trí lại để tránh mép bo/courtyard module. Tụ gốm còn lại dùng P5, không có tụ 0805 trên carrier; cần xác nhận kích thước/đặc tính hàng mua.
- ESP32 xoay180° so với bố trí trước: USB-C bên trái/anten bên phải hướng ra mép ngoài. Tên chân được dịch theo pad; pinout/net không đổi. C4/C5 chuyển theo đầu5V/GND mới. Vùng cấm đồng/via/track/pad x196,5..210,y50..88,5 trên hai mặt dành cho đầu anten. Đây là dự trữ theo hình học hiện có, chưa chứng minh đủ khoảng15mm với toàn bộ module/khung xe; không tự cắt nền hoặc mở rộng tránh pad khi chưa đo hình học anten thật.

Không đưa dòng motor qua chânGND củaBMI hoặc giắcencoder như một điểm nối tiếp. Tuy vậy plane chung vẫn là đường dẫn phân bố; khẳng định miễn nhiễu/chạy bền cần thử tải thực tế, không chỉ nối đủ net.

## Kết quả kiểm tra

- ERC:0 vi phạm.
- DRC:0 vi phạm,0 kết nối chưa đi dây.
- Schematic parity:0 vấn đề.
- CHECK_erc_final.json, CHECK_drc_final.json và ROUTING_CHECKS.json lưu kết quả/dấu nguồn.
- ROUTING_OVERVIEW.png minh họa track hai mặt, ẩn plane để đọc đường dây; không phải ảnh3D hoặc ảnh sản phẩm. SVG cập nhật chứa lớp đồng và các chú thích.

DRC đã bắt và sửa5lỗi ban đầu: mépđồng thiếu0,005mm; hai đoạn còi0,15mm dưới mức0,2; thermal bị thiếu tại hai chânGND driver. Không hạ quy tắc DRC hoặc bỏ kiểm tra để đạt0.

## Những việc còn phải xác nhận trước khi đặt sản xuất

1. Các sốđo module đã dùng nhưng offsetmép, tiết diện chân/drill1mm, lỗgá/chiều cao chưa được người dùng xác nhận đầy đủ. Thử bảnin1:1 đúng hướng component-side; đặc biệt BMI5+7 và modulehàn trực tiếp. Không dùng hình phối cảnh để xác nhận kích thước này.
2. Khoảng cắm USB và khoảng hởanten so vớiSTM32/khung xe. Không coi vùngkeepout tạm là chứng nhận RF. Cần kiểm tín hiệuWiFi thực với xe lắp hoàn chỉnh.
3. Chưa xác minh dòng/loại còi nốiPB12; giữ mạch người dùng đang chạy, chưa tự thêm transistor. Chưa xác minh pullup trênGY-BMI160, không thêm kéo lên trùng hoặc đưaI2Clên5V. Điện trởR250 trênmoduleA4950 không được nhân đôi trêncarrier.
4. Chọn chính xác đầu nối chịu dòng, tụđúng footprint/DCbias/ripple/ESR. GiữVCC driver5V theo hướng dẫn; không suy ra giới hạn dòng2A khi chưa có sơ đồmodule.
5. Kiểm nguồnXL4015 khởi động vớiC1=1000uF và tảiESP/WiFi, VM lúcmotor phanh/đảo, nhiệt đườngđồng/đầu nối và mức3V3 tạiBMI. Không có sốđo dòng cực đại để chứng nhận định mức dòng/nhiệt cho bo này. Chưa xuấtGerber sản xuất.

## Tài liệu sử dụng

- KiCad PCB Editor, netclasses/zone/DRC/Specctra: https://docs.kicad.org/10.0/en/pcbnew/pcbnew.html
- Freerouting local CLI: https://github.com/freerouting/freerouting/blob/master/docs/command_line_arguments.md
- Espressif, module positioning/antenna clearance và nguồn: https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32s3/pcb-layout-design.html
- Allegro A4950 application information (phân biệtICvớimodule): https://www.allegromicro.com/-/media/files/datasheets/a4950-datasheet.pdf

Nguồn chỉ dẫn nguyên tắc bố trí/đi dây; không chứng minh các ngưỡng cơ khí, tải và đáp ứng nguồn riêng của xe đã đạt. Không đưa thiết kế vào sản xuất chỉ vì ERC/DRC/parity bằng0.

## Chốt cập nhật theo BOM ngày 01/11/2026

C1 đổi đường bao sang Ø8/P5/H16 theo link shop; C4/C6 đổi tụ phân cực Ø4/P2/H7, pad1=+5V và pad2=GND, giữ 10 µF. C4 đặt ở bên trái đầu nguồn ESP32, C6 ở bên phải cụm STM32 để có đủ khoảng hở thân tụ. C5/C7/C9 ghi 100 nF/50 V, C8 ghi 1 µF/50 V theo link BOM. Đã đi dây lại, đổ đồng và xem bản vẽ xuất từ PCB. ERC=0, DRC=0, parity=0, unconnected=0; netlist so với trước thay tụ không đổi; 18 footprint/148 pad PTH/0 pad SMD. Không đổi firmware. Kiểm tra này không xác nhận ESR/ripple hoặc kích thước mẫu tụ gốm shop chưa công bố. Bản xuất ảnh chỉ để xem bố trí, không phải Gerber sản xuất.

## Tăng kích thước bo và thêm lỗ gá tạm

PCB 160×65 mm, góc trên trái (50;50), dưới phải (210;115) trong KiCad. H1–H4 đặt cách mép 4,5 mm, lỗ không mạ Ø3,2 mm; vùng tránh đồng Ø8 mm hai lớp. Đã dịch bố trí và đường dây (+3,5;+4) mm, riêng BZ1 thêm (+1,5;0) mm để đủ khoảng vít H4. Giữ các net và bề rộng dây; đổ lại GND theo mép mới và kéo vùng cấm đồng anten đến mép phải mới. Schematic điện không đổi, lỗ cơ khí đặt thuộc tính board-only. DRC/parity/unconnected=0; tọa độ lỗ và khoảng RF với bộ gá còn chờ số đo thực tế.

Cập nhật khoảng hở H4: BZ1 dịch thêm 2 mm sang phải trong KiCad, tâm mới (64;107) mm. Giữ nguyên bốn tâm M3; chỉnh dây và đổ lại GND. DRC/parity/unconnected=0. Với đường bao BZ1 hiện tại, đầu vít Ø6,4 mm còn khoảng 1,75 mm; vòng đệm Ø8 mm còn khoảng 0,95 mm tới góc gần nhất.

## ESP32: thân 57 mm, tổng dài 64 mm tính cả anten

Người dùng xác nhận: 57 mm chỉ là thân có hàng chân, không tính lưỡi anten; lưỡi nhô thêm 7 mm, tổng dài 64 mm. Thông tin này thay thế các nhận định trước đây coi 57 mm là toàn bộ module. Giữ chiều rộng thân 28 mm và toàn bộ 44 pad/vị trí chân/net/UART. Gốc footprint vẫn ở tâm thân 57 mm, không chuyển sang tâm đường bao 64 mm.

Theo chiều lắp hiện tại, USB bên trái, anten bên phải: thân ở x140..197/y58,5..86,5 mm; lưỡi anten ở x197..204 mm. Đường bao anten đang dự trữ rộng28 mm bằng thân vì chưa có số đo bề rộng lưỡi riêng. PCB vẫn160×65 mm, mép phải x210 mm, lưỡi còn cách mép phải6 mm; chưa phải bố trí anten nhô khỏi mép carrier. Vùng cấm đồng x196,5..210/y50..88,5 đã phủ phần lưỡi mới và kéo tới mép; đây không phải chứng nhận RF.

Courtyard đã mở rộng cho tổng64 mm. H2 và các linh kiện không vi phạm courtyard theo đường bao đang dùng; vẫn cần kiểm vít/cọc và chất lượng Wi-Fi thực tế. DRC/parity/unconnected=0, toàn bộ pad/net/đường dây không đổi. Không sửa firmware hoặc dấu IMU/motor.

## Phân phối nguồn thay WAGO trên carrier

Đã đi lại riêng nhánh pin từ J1 tới VM/IN+ buck và ba nhánh5V từ vùng C1 tới ESP/STM/driver; thêm hồi công suất về J1 và hồi từ OUT− tới GND2 driver. Giữ plane GND chung, không phải star-ground một điểm tuyệt đối. Xem POWER_DISTRIBUTION.md để lắp không WAGO và hiểu giới hạn. Toàn bộ module/pad/net và đường UART/I2C/PWM/encoder giữ nguyên. DRC/parity/unconnected=0; chưa xác nhận ổn định thực nghiệm hoặc định mức dòng/nhiệt.

## Lối cắm cáp nạp — cập nhật02/11/2026

ESP32 xoay180° so với bản trước, đặt tâm thân ở(181,5;72,5)mm. USB-C hướng phải, mặt cổng ở cạnh carrier x210mm; anten sang trái, x146..153mm. Vùng cấm đồng anten đổi sang x131..153/y50..88,5mm; không phải chứng nhận RF nhưng không đặt linh kiện dưới lưỡi anten. Giữ nguyên tên/pad44chân, cập nhật vị trí chữ theo chiều mới.

C6 chuyển xuống dưới thân STM, tâm thân tụ(153;112,5)mm; khu vực bên phải STM x195..210/y94,5..104,5mm không có tụ để dành cho đầu SWD/ST-LINK cắm ngang. USB ESP dành lối tiếp cận ngoài mép phải x210..230/y58,5..86,5mm. Đây là vùng dự trữ cho đầu cáp; kích thước/chiều cao housing thực chưa được đo. Không cần đổi SWD thành USB/DFU của STM hoặc sửa firmware.

Để tránh vùng anten mới, BMI160 chuyển sang(100;89,25)mm, **không xoay hướng cảm biến**; J3 chuyển sang(77,5;96)mm; C2/C4/C5/C8/C9 điều chỉnh cùng bố trí. Năm module vẫn nằm trên PCB, bốn lỗ M3 giữ nguyên. Net điện và giá trị tụ không đổi; đi dây lại theo vị trí mới, giữ các nhánh phân phối nguồn từ J1/C1 và GND chung. Các đường tín hiệu được đi lại, không dùng chiều dài/khoảng hở của bản01/11 để đánh giá bản này.

Đã kiểm DRC/parity/unconnected=0. Đây là kiểm tra CAD, chưa chứng minh cáp thực cắm vừa hoặc xe ổn định điện/EMI. In PDF mới1:1 và đối chiếu cả đầu nhựa cáp, hướng cắm SWD và chiều cao module. Bản PDF/layout cũ không còn đúng bố trí hiện hành.

## Anten ngoài, BMI giữa, chừa USB —02/11/2026

Thay thế phương án anten quay vào trong. ESP32 tâm thân(181,5;72,5)mm, góc180°: thân x153..210mm, lưỡi anten x210..217mm nhô hẳn ngoài cạnh phải carrier. USB-C hướng trái, mặt cổng ởx153; hành lang trước cổng **20mm**, x133..153/y58,5..86,5mm, không có courtyard linh kiện khác. Phải đối chiếu vỏ đầu cáp thực tế; khoảng20mm là vùng dự trữ thiết kế, không chứng nhận mọi kiểu đầu cáp.

BMI160 trở lại vùng giữa như bố trí cũ: tâm(123,5;82)mm, lệch tâm hình học bo(130;82,5) khoảng6,5mm sang trái để chừa hành lang USB. Giữ nguyên hướng7chân=tiến/5chân=lùi, không đảo trục hoặc dấu firmware. J3/C2/C8/C9 về bố trí cũ; C4/C5 nằm trên đầu nguồn ESP; C6 vẫn ở dưới STM, tránh SWD bên phải.

Vùng anten cấm đồng/đường dây x209,5..232/y43,5..101,5mm bao phần nhô ngoài bo và dải mép. Giữ bốn tâm M3; không tự cắt carrier hoặc bỏ lỗ. Vẫn cần kiểm anten với khung/vít và Wi-Fi thực tế, không coi DRC là kiểmRF. PCB160×65mm, bao ngoài có anten rộng thêm7mm thành167mm theo chiều ngang này.

Các net/pin nguồn/tín hiệu giữ nguyên, đi dây lại theo vị trí mới và giữ phân phối nguồn J1/C1, GND chung. DRC/parity/unconnected=0, đã kiểm cả courtyard trong hành lang USB và SWD. Không dùng tọa độ/chiều dài đường dây của phương án anten vào trong để đánh giá bản này. PDF1:1 mới bao gồm cả lưỡi anten nhô ngoài bo; bản in cũ hết hiệu lực.

## Black Pill lệch thân so với hàng chân —02/11/2026

Người dùng đo: mép đầu5V/PB12 tới tâm chân đầu1mm; mép đầuVBAT/3V3 tới tâm chân cuối3mm. Giữ20chân/hàng, bước2,54mm, hai hàng15,24mm. Khoảng tâm đầu-cuối48,26mm suy ra chiều dài thân52,26mm; thay số53mm danh nghĩa trước đây để khớp hai mép mới. Chưa phải phép đo độc lập xác nhận chiều dài52,26mm, cần kiểm bản giấy.

Giữ nguyên toàn bộ tọa độ lỗ/pad và net. Sửa đường bao thân/courtyard: thân x143,37..195,63/y89,5..109,5mm, tâm thân(169,5;99,5), lệch1mm sang phải so với tâm hàng chân(168,5;99,5). Như vậy từ mép thân đầu5V/PB12 tới tâm144,37 là1mm; cuối192,63 tới mép195,63 là3mm. Không thay pitch để bù bản in bị thu nhỏ. Chiều rộng20mm giữ nguyên. Vùng SWD bên phải bắt đầu x196mm thay cho195,5mm; C6 vẫn ở dưới STM.
