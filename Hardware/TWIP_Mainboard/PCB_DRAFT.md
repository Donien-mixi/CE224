> Bản PCB ngày 06/11/2026 dùng một mặt đồng B.Cu, 31 đoạn dây cầu trong 24 nhóm. Tâm thân BMI160 tại (130,00; 81,65) mm. Xem [SINGLE_LAYER.md](SINGLE_LAYER.md) và [bảng dây cầu](SINGLE_LAYER_WIRES.csv) cho bản lắp hiện hành. Các tọa độ bố trí và số dây của bản trước bên dưới là dữ liệu lịch sử.

> KÍCH THƯỚC MODULE HIỆN HÀNH: xem MODULE_DIMENSIONS.md. Đã cập nhật theo mép chân mới02/11; số57/64 ESP,18×13 BMI thẳng cột,23×18 driver và XL51×21 trong lịch sử đã được thay thế một phần.

> STM32: đường bao thân theo mép1mm/3mm mới, chiều dài suy ra52,26mm; giữ nguyên lỗ và hàng chân. Xem cập nhật cuối tài liệu.

> HIỆN HÀNH: anten ESP32 nhô ngoài cạnh phải7mm, USB hướng trái chừa20mm; BMI về vùng giữa cũ. Phương án anten vào trong đã thay thế. Xem mục cuối tài liệu.

> BỐ TRÍ HIỆN HÀNH02/11: ESP USB-C ở cạnh phải, C6 ở dưới STM; xem mục lối cắm cáp cuối tài liệu. Các tọa độ/độ dài đường dây bản cũ đã thay đổi.

> NGUỒN HIỆN HÀNH: phân nhánh trên PCB thay WAGO; xem POWER_DISTRIBUTION.md. Các mô tả đường nguồn trước đó là lịch sử.

> ESP32 HIỆN HÀNH: thân57 mm + lưỡi anten7 mm = tổng64 mm. Xem mục cập nhật cuối tài liệu; các ghi chú cũ coi57 mm là toàn bộ module đã được thay thế.

> CẬP NHẬT LỖ GÁ: tâm H1–H4 đã theo tọa độ người dùng (cm), không còn vị trí tạm cách mép 4,5 mm. Xem MOUNTING_HOLES.md. Hướng tiến = hàng 7 chân BMI160, hướng lùi = hàng 5 chân. Các tọa độ/lỗ tạm bên dưới là lịch sử.

> KÍCH THƯỚC HIỆN HÀNH: 160 × 65 mm, 4 lỗ M3 tạm H1–H4. Xem MOUNTING_HOLES.md và ROUTING_REVIEW.md. Các kích thước/vị trí cũ bên dưới là lịch sử.

> Cập nhật CAD hiện hành 01/11/2026: xem BOM_THT.md và ROUTING_REVIEW.md. C1 D8/P5; C4/C6 tụ hóa phân cực D4/P2. Các ghi chú bố trí/0805/0-track trước đây bên dưới là lịch sử.

> CẬP NHẬT HIỆN HÀNH: toàn bộ carrier đã đổi sang THT. C3–C9 là MLCC chân5mm, không còn0805. Đã đi dây lại và kiểmERC/DRC/parity0;148padPTH,0padSMD. Xem BOM_THT.md. Các mô tả0805/phương án cũ bên dưới là lịch sử, không dùng mua linh kiện.

# PCB TWIP — bố trí P0, chưa đủ điều kiện sản xuất

Ngày31/10/2026. File chính `TWIP_Mainboard.kicad_pcb`, KiCad10.0. **Chưa hoàn tất yêu cầu đi dây:** thiếu xác nhận phiên bản/pad map của module thực tế. Không đặt PCB/Gerber từ bản này.

## Những gì đã làm

- Đọc Hardware README, symbol library, schematic rev1.3; export netlist bằng KiCad CLI. Có18 phần tử:5module,9tụ,3giắc và còi. Net chuẩn giữ từ schematic, không đổi firmware hay sơ đồ nguyên lý.
- Board2lớp, biên Edge.Cuts từ(50,50)đến(200,110)mm: **150×60mm** tính theo tâm đường biên.
- STM32 ở trái, BMI160 gần giữa, ESP32 bên phải, driver phía trước vùng công suất và XL4015 phía sau trên cùng PCB. Việc chuyển XL4015 lên PCB thay đổi phân bố khối lượng/cao độ của xe; mô phỏng cũ chưa tự cập nhật theo bố trí mới này.
- Gán sơ bộ18footprint/bao hình trong thư viện portable `TWIP_Carrier.pretty` và `fp-lib-table`. U2 là template Black Pill WeAct, connector/cap là lựa chọn đề xuất cần mua đúng linh kiện. U1/U3/U4/U5 chỉ là bao hình có ghi VERIFY, **không có pad điện do chưa biết thứ tự chân thật**.
- Các đường mảnh trên Dwgs.User chỉ là kế hoạch liên kết logic, **không phải copper trace**. Có0track đồng, chưa đổ ground.
- `NET_CONNECTIONS.csv` lưu toàn bộ reference/logical pin/net từ schematic; `PLACEMENT_DRAFT.json` ghi tọa độ/kích thước/trạng thái. Không đổi tên chân logic thành số chân vật lý bằng suy đoán.
- KiCad DRC cuối: **0vi phạm hình học/clearance trên phần đã dựng, 32mục chưa nối**. Vì4module chưa có pad, số32 còn không bao quát các kết nối vật lý bị thiếu; không gọi là DRC hoàn chỉnh hoặc nghiệm thu điện. Chưa kiểm parity schematic hoàn chỉnh/chưa có Gerber sản xuất.

## Kích thước đang dùng và độ tin cậy

| Ref | Phần | Bao hình mm | Nguồn/trạng thái |
|---|---|---:|---|
| U2 | Black Pill |20,78×52,81|Bản vẽ WeAct2x20; pitch2,54mm; khoảng2hàng15,24mm. Cần đối chiếu revision bo thật/USB/SWD/mặt dưới|
| U3 | ESP32-S3 dev board |25×60|Ngân sách từ thông số thiết kế cũ của dự án, chưa xác minh mã bo cụ thể. N16R8 là dung lượng chip/module, không xác định kích thước dev board|
| U4 | Dual A4950 |25×20|Bao hình dành chỗ theo dữ liệu dự án, không phải footprint chip A4950 hay pad map module đã chứng minh|
| U5 | GY-BMI160 |18×14|Bao hình dành chỗ, khác bảng15×15mm cũ; chưa chốt kiểu đầu cắm, CS/SA0/INT1/VIN và mặt đặt|
| U1 | XL4015 |52×25|Bao hình từ mô hình cơ khí dự án, chưa xác minh loại52×25/54×23 hoặc boCC/CV lớn hơn, lỗ bắt, vị tríIN/OUT và chiều cao heatsink|

Các khối chưa xác minh chỉ phục vụ sắp xếp. Không được lấy outline hoặc template này làm giấy khoan cho module thật. Ảnh preview không chứng minh lắp vừa. U3 hiện chiếm gần hết chiều60mm: cần xác minh USB/antenna/đầu camera/cáp và chừa thao tác, có thể phải xoay/di chuyển sau đo.

## Thông tin cần người dùng cung cấp để đi dây đúng

Cho mỗi module đang dùng: ảnh rõ mặt trên/mặt dưới có chữ chân, tên/link mua hoặc mã bo chính xác, chiều dài/rộng, số chân mỗi hàng, pitch, khoảng cách tâm hai hàng và vị trí lỗ bắt. Đặc biệt cần **ESP32-S3, Dual A4950, GY-BMI160, XL4015**; Black Pill cũng xác nhận đúng40chân WeAct tương ứng. Ảnh kèm thước/cạnh thước giúp đối chiếu; không cần tháo chip.

Cho khung xe: hướng đặt PCB150×60, khoảng cách tâm lỗ/cọc, khoảng trống mặt dưới và độ cao module/cáp. Các vòng tròn Dwgs.User là gợi ý vị trí, chưa có lỗ khoan cơ khí. Driver cần xác nhận dòng/rating connector; các giắc1x6đề xuất không được tự coi chịu đủ dòng stall motor.

## Nguyên tắc đi dây sau khi có pad map

1. Pin3S→driverVM đi ngắn/rộng, tụC2/C3 sát chânVM/GND thực; dòng motor hồi trực tiếp về nguồn, tránh chạy qua vùngGND MCU/IMU. Không tự chia đất làm đứt đường hồi tín hiệu; plane cần liên tục và bố trí vòng dòng đúng.
2. XL4015 ở vùng công suất, giữ đường5V đếnESP và tụC1/C6/C7 ngắn/rộng; chỉnh và đo5,0V trước khi cắmlogic. Không đi nút switching bên trong buck hoặc dòngmotor dưới IMU/antenna.
3. BMI160 đặt chắc cơ khí, trục phù hợp quy ước firmware, I2C ngắn và có đườngGND hồi; xác minh pull-up sẵn trênmodule trước thêm. C8/C9 sát3V3/GND module.
4. ESP antenna hướng ra mép, không copper/track/via/linh kiện dưới antenna. Vùngkeepout hiện tại là giả định chờ xác định đúngantenna, chưa đủ chứng minh RF. Kiểm15mm theo hình hướng dẫn của hãng, cả pin/kim loại khung/cáp lân cận.
5. Chọn tracewidth/copperthickness theo dòng thực/độ tăng nhiệt, không mặc định0,25mm cho VM/motor. Chốtpartnumber/rating capacitor/connector trướcfab. C1 radialD8/P3,5 và C2 tương tự là đề xuất; phải khớp thân/chân/cao độ tụ thật. C0805 cần đủ điện áp và xét giảm điện dung theoDCbias.
6. Schematic hiện nối còi hai chân trực tiếpPB12/GND. Cần dòng/điện áp còi để kiểm giới hạn GPIO và xác định có cần transistor/diode; chưa tự thêm/sửa schematic. Không xem ERC0 là bảo đảm tải GPIO an toàn.
7. Gán footprint tương ứng và pad mapping vào schematic, updatePCB, route/plane, DRC+ERC+schematic parity, kiểm netlist độc lập và in1:1 thửmodule. Chỉ sau đó xuấtGerber/drill.

## Nguồn đã tra

- WeAct: https://github.com/WeActStudio/WeActStudio.MiniSTM32F4x1/tree/master/Hardware — bản vẽ module, không dùng footprintLQFP của chip để thay cảBlackPill.
- Bản vẽ kích thước WeAct lưu tại STM32-base: https://stm32-base.org/assets/pdf/boards/original-dimensions-STM32F411CEU6_WeAct_Black_Pill_V2.0.pdf .
- Espressif antenna/baseboard: https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32s3/pcb-layout-design.html . Tài liệuWROOM không xác định pin header của mọi devboardN16R8.
- XLSEMI: https://www.xlsemi.com/datasheet/XL4015-EN.pdf — IC/bypass/layout guidelines, **không phải** bản vẽ cơ khí của module bán sẵn.
- Allegro: https://www.allegromicro.com/~/media/Files/Datasheets/A4950-Datasheet.pdf — IC/decoupling/current path, **không phải** footprint boDualA4950.

Chưa tự sản xuất/nạp; yêu cầu PCB routed hoàn chỉnh đang chờ dữ liệu vật lý nêu trên.

## Cập nhật mặt bằng 01/11/2026

Kích thước thân module do người dùng cung cấp: ESP32 64×28 mm, STM32 53×20 mm, A4950 23×18 mm, BMI160 18×13 mm, XL4015 54×24 mm. Dữ liệu và vị trí mới trong PLACEMENT_DRAFT.json thay thế kích thước/bố trí sơ bộ cũ bên dưới.

ESP32 và STM32 đặt song song theo cạnh150 mm; XL4015 ở đầu trái bản vẽ; BMI160 gần tâm PCB; A4950 phía dưới BMI160. Đây là trái/phải trên bản vẽ, chưa gán lại hướng trước/sau vật lý. PCB vẫn lắp theo breadboard. Đã bỏ nhãn FRONT cũ chưa xác nhận.

Đã cập nhật5 đường bao thân và thư viện, giữ18 footprint/74pad/0track. Kiểm tra thân5module nằm trong150×60 mm, không chồng nhau; DRC0vi phạm/32unconnected của các pad hiện có. Bốn module chưa có pad map; con số32 không biểu thị đầy đủ kết nối còn thiếu. Kích thước thân không xác nhận vị trí chân, lỗ bắt vít, khoảng cách cắm USB, camera, chiều anten hoặc thông gió XL4015. Keepout anten bên phải ESP là dự kiến, cần đối chiếu ảnh. Các đầu nối/tụ nhỏ cũng mới là vị trí dự kiến. Chưa dùng để sản xuất.

PLACEMENT_OVERVIEW.png là hình minh họa chỉ5thân module, không phải ảnh copper hoặc mô hình3D. PLACEMENT_PREVIEW.svg xuất từ PCB đầy đủ.

## Header đo thực tế được cập nhật

Người dùng cung cấp STM32 2x20/hai hàng15,24; ESP32 2x22/hai hàng25,5; A4950 2x7/hai hàng20; BMI160 theo ảnh2x6/hai hàng10; XL4015 bốnpad51x21 mm. Chọn hàn trực tiếp chân module vào lỗ PCB, không dùng đế cái. Đã tạo footprint cơ khí và cập nhật PCB/thư viện; U2 giữ nguyên. Các header được giả định đặt giữa thân, chưa có offset mép. Bốn footprint mới dùng sốpad cơ khí tạm, chưa gán net; không dùng sốnày để tự động cập nhật từ sơ đồ logic. Lỗ1mm/pad1,8mm là lựa chọn sơ bộ, chưa có sốđo tiết diện chân.

DRC hiện báo8padESP nằm trong vùng keepout anten dự kiến cũ. Đây là xung đột phải giải quyết sau khi xác định đầu anten thực, không được bỏ keepout/gỡkiểmtra để gọi PCB đãđạt. Con số32unconnected không đếm các padchưa gánnet. Chưa xuất Gerber sản xuất. MEASURED_HEADERS.json là nguồn sốđo mới, thay trạng thái cũ0pad của4module trong bảnplacementJSON trước. Mã stm32f411ceu6/GQ21PLTVQ/CHNGQ546 người dùng ghi là markingIC, không đủ xác định revisionPCBBlackPill.

ESP32 correction from owner: body length57mm (previous64mm superseded), width28mm,2x22 pins/pitch2.54mm/rows25.5mm unchanged. Only body outline/courtyard changed; actual edge offsets and pin map remain unverified. Old overview PNG removed from current use; SVG is current.

BMI160 correction: owner confirms5+7 pins, NOT2x6. Current U5 footprint GY_BMI160_5plus7_Row10_VERIFY_MAP uses10mm row spacing;5pin and7pin rows provisionally centered. Exact longitudinal alignment and physical signal map remain unverified. Prior2x6 statements are superseded.

ESP32 pin labels updated from owner image: antennaLEFT/USB RIGHT in PCB view,44labels plus logical pad names and U3 source nets. ESP32_OWNER_PIN_MAP.json records full transcription. Removed obsolete right-end assumed antenna rule area; antenna is now identified atLEFT but actual antenna-zone geometry remains unverified. Do not route/fabricate before defining proper keepout including nearby modules. Other mechanical drill/pin map checks remain pending.

ESP32 owner photo: actual dualUSB-C module, antenna at opposite end; PCB top view antennaLEFT/USB-C RIGHT. Added illustrative shield/antenna/USB/button outlines on F.Fab. Dimensions/offsets of these internal graphics are NOT measured manufacturing geometry and do not define RF keepout or USB plug access. Body57x28 and all44pad positions/names/nets unchanged, verified before/after. Module/camera/connectors physical clearance remains pending.

BMI160 owner image resolves pin names and5/7-row alignment. Backside letters mirrored to component-side PCB view; upper5=OCS INT2 INT1 SCX SDX; lower7=VIN3V3 GND SCL SDA CS SA0. Logical pad names/nets now match schematic; auxiliaryOCS/SCX/SDX left unconnected. OCS distinct fromCS. Centered5/7 alignment now confirmed visually, absolute edge offsets/drill still provisional.

## Cập nhật tụ lọc theo hướng dẫn đi dây

Đã chuyển C1 cạnh đầu ra XL4015; C2/C3 về VM/GND trái A4950; C4/C5 về5V/GND đầuUSB ESP32; C6/C7 về5V/GND cuốiSTM32; C8/C9 ngay hàng nguồnBMI. Khoảng cách tâm tụ đến padnguồn gần nhất mới (mm): C1=12,75; C2=12,39; C3=4,28; C4=3,92; C5=5,39; C6=4,10; C7=6,41; C8=3,06; C9=4,90. Đây không phải chiều dài dây đồng hay chứng minh hiệu quảlọc. Không bố trí tụ lọt vào đường bao module; giữ khoảng sân linh kiện theoDRC.

C1/C2 đổi đề xuất sang radialD10/P5, cao20mm. Mã gợiý mua đúngcase: NichiconUHE1C102MPD1000uF16V vàUHE1V471MPD470uF35V; không khẳng định đã có các tụ này trên xe. Nguồn hãng: https://www.nichicon.co.jp/english/series_items/catalog_pdf/e-uhe.pdf và https://www.nichicon.com/en-us/part/uhe1v471mpd/5835/ . Phải đối chiếu case/lead-forming mã đặt hàng; không dùng kích thướcØ8/P3,5 cũ. Dấu+ bổ sung trênSilkscreen. Giữ nguyên9giá trịtụ/net, không bổ sung điện trở/chuyển mạch còi khi chưa biết tải. I2Cpullup chưa xác minh nội bộmodule nên chưa thêm trùng.

U1 cũng đã gán tênpad IN+/IN-/OUT+/OUT- vànet nguồn theo schematic hiện có, thay sốcơkhí1..4 chưa cónet. Không đổi topology. PCBvẫn0track; DRCkhôngvi phạm không có nghĩa đã nối điện hoặc đủ điều kiện sảnxuất. Giá trịtụ cầnkiểm nghiệm vớiXL4015 vàmotor thật; RFkeepout/khoảng hởUSB/lỗgá vẫn chưa chốt. CAPACITOR_PLACEMENT.json là vịtrí tụ hiện hành.

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
