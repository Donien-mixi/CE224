> Bản PCB ngày 06/11/2026 dùng một mặt đồng B.Cu, 31 đoạn dây cầu trong 24 nhóm. Tâm thân BMI160 tại (130,00; 81,65) mm. Xem [SINGLE_LAYER.md](SINGLE_LAYER.md) và [bảng dây cầu](SINGLE_LAYER_WIRES.csv) cho bản lắp hiện hành. Các tọa độ bố trí và số dây của bản trước bên dưới là dữ liệu lịch sử.

> KÍCH THƯỚC MODULE HIỆN HÀNH: xem MODULE_DIMENSIONS.md. Đã cập nhật theo mép chân mới02/11; số57/64 ESP,18×13 BMI thẳng cột,23×18 driver và XL51×21 trong lịch sử đã được thay thế một phần.

> HIỆN HÀNH: anten ESP32 nhô ngoài cạnh phải7mm, USB hướng trái chừa20mm; BMI về vùng giữa cũ. Phương án anten vào trong đã thay thế. Xem mục cuối tài liệu.

> BỐ TRÍ HIỆN HÀNH02/11: ESP USB-C ở cạnh phải, C6 ở dưới STM; xem mục lối cắm cáp cuối tài liệu. Các tọa độ/độ dài đường dây bản cũ đã thay đổi.

# Kiểm tra PCB và kế hoạch kiểm thử trên xe

Ngày 01/11/2026. Áp dụng bản carrier160×65mm, hai lớp, có bốn lỗ M3 theo số đo người dùng. Chưa phê duyệt chế tạo/độ ổn định thực tế.

## 1. Kết luận đối chiếu breadboard

Phân chia **đúng về kết nối điện**, nhưng **chưa tách rạch ròi toàn bộ về không gian và đường hồi dòng** như các bó dây riêng trên xe. PCB không cách ly nguồn/tín hiệu về điện; mọi khối dùng chung GND như tài liệu gốc.

| Nhóm | Thực hiện trên PCB | Phạm vi xác nhận |
|---|---|---|
| Pin3S | J1 tách đến VM driver và IN+ XL4015; đường1,5mm | Đúng rail, không cấp pin3S cho MCU/BMI |
| 5V | Vùng C1/OUT+ buck chia tới ESP/STM/VCC driver;0,7mm, nguồn tới C1=1mm | Không dùng đường motor để cấp nguồn logic; còn đường đi vòng dài |
| 3V3 | Từ STM tới BMI/CS và encoder;0,7mm | VIN BMI bỏ trống, SA0 xuống GND, giữ cách cấp nguồn đang dùng |
| Driver | Ngõ ra AOUT/BOUT đi riêng đến dây motor; GND1 có đường rộng về J1− | Đúng net, chưa có đo dòng/nhiệt/ground bounce |
| GND | Plane chung hai mặt và các nhánh hồi rộng | Không phải star-ground một điểm; không thể khẳng định dòng motor tuyệt đối không phân bố qua vùng khác |
| Tín hiệu | UART/PWM/I2C/encoder giữ pinout firmware | Một số đường encoder phải còn sát/cắt hình chiếu đường motor |

Schematic mới xuất có80 node trong netlist; tất cả tìm được đúng pad/net trên PCB (một số tên nguồn có pad lặp trên module). DRC=0, parity=0, unconnected=0. Đây là kiểm tra kết nối/khoảng hở, không kiểm chứng EMC hoặc chất lượng nguồn.

## 2. Các điểm còn phải xử lý/kiểm chứng

Số dưới đây tính từ đoạn track hiện có, không gồm toàn bộ ảnh hưởng pad/via/dây ngoài/module. Khoảng cách là mép track cùng lớp, không phải tiêu chuẩn pass/fail nhiễu.

| Tín hiệu | Chiều dài track tổng | Khoảng hở nhỏ nhất tới track ra motor cùng lớp | Nhận xét |
|---|---:|---:|---|
| I2C SCL |67,30mm|8,06mm|Không có giao hình chiếu centerline với motor khác lớp trong phép tính này |
| I2C SDA |67,30mm|10,10mm|Tương tự SCL |
| UART STM→ESP |105,00mm|18,30mm|PB6→GPIO18 đúng;3via |
| UART ESP→STM |106,47mm|19,24mm|GPIO17→PB7 đúng;3via |
| Encoder phải A |83,59mm|**0,275mm**|Có4giao hình chiếu centerline với track motor ở lớp kia; ưu tiên xem lại phân vùng |
| Encoder phải B |97,25mm|**0,453mm**|Có2giao hình chiếu centerline khác lớp; ưu tiên xem lại |
| Encoder trái A/B |117,07/111,55mm|5,84/4,06mm|Dài, cần kiểm dạng xung tại đầu vào MCU |

Giao hình chiếu khác lớp không phải nối tắt và không tự chứng minh có lỗi; khoảng cách đạt DRC cũng không tự chứng minh miễn nhiễu. Cần xem đồng hồi bên dưới và dạng xung khi motor đóng cắt.

Nhánh chính C1→STM theo đường đi đã lập dài khoảng**229,9mm**, có4chuyển lớp; C1→VCC driver khoảng129,3mm. Phân nhánh nguồn đã có nhưng chưa tối ưu độ ngắn. Ví dụ riêng đường đồng0,7mm dài229,9mm với **giả định** đồng35µm,20°C có điện trở xấp xỉ0,162Ω; đây không phải điện trở đo và chưa gồm via/đầu nối/đường hồi. Không kết luận sụt áp thực khi chưa biết dòng và độ dày đồng. Nên tối ưu đường đi này và encoder phải trước khi chốt sản xuất.

Vẫn còn các mục cơ khí đã ghi riêng: mép lỗ M3 còn0,9mm, đầu vít/vòng đệm có thể nhô; kích thước tụ gốm hàng mua, offset chân module và bề rộng lưỡi anten chưa xác nhận đầy đủ. Không dùng DRC để bỏ qua các mục này.

## 3. Vì sao chưa thể kiểm chứng PCB bằng mô hình Scilab hiện tại

Đường vào `robot_can_bang_pid_1.sce` chạy `Simulation/current/review.sce`, dùng mô hình thân xe/motor/observer và các DLL native. Mô hình có thể thay điện áp VM, độ trễ, ma sát và nhiễu IMU theo tham số. Nó không nhập đường đồng/zone KiCad, không giải mạng GND/EMI, không mô hình đầy đủ buck XL4015, LDO devkit, ESR/ESL tụ, sụt nguồn/reset MCU hoặc lỗi bit UART/I2C do nhiễu.

VM trong `Simulation/config/motor_supply.sce` là mức tham chiếu11,7V lịch sử; không phải tín hiệu nguồn đo trên PCB. Firmware nguồn hiện khai báo RT13-RESP95; tên DLL review có rt5 không tự chứng minh nội dung cũ hay mới, cần đối chiếu source/build/hash trước khi coi là mô phỏng firmware đang nạp.

**Chưa chạy một ca cân bằng rồi gắn nhãn PCB đạt.** Giữ nguyên mô hình và PID trong lượt kiểm tra này. Mô phỏng xe ổn với nguồn lý tưởng không phát hiện được reset hoặc nhiễu do PCB.

Có thể mở rộng thành kiểm tra có điều kiện sau khi có dữ liệu: đưa VM(t) đo tại driver vào mô hình lực motor; đưa sai số/mẫu mất IMU đã phân biệt với rung cơ khí vào mô hình đo; phát lại lệnh với trễ/mất frame đo được. Đầu ra chỉ kiểm độ chịu đựng điều khiển với các nhiễu đó, không chứng nhận EMI hoặc toàn bộ PCB.

Muốn mô phỏng mạch nguồn trước khi có bo, cần chốt stackup/độ dày đồng, dòng motor có tải và các tải5V/3V3, sơ đồ/regulator của module thực, thông số buck/LDO/tụ/đầu nối và mô hình điện ký sinh. RLC/SPICE/Xcos có thể khảo sát mạng đã xây dựng; kết quả phụ thuộc chất lượng các tham số này, không thay thế đo trên bo.

## 4. Kiểm thử sau thiết kế và chế tạo

Ghi lại build STM/ESP, điện áp pin thực, độ dày đồng/vật liệu bo, linh kiện đã lắp, điều kiện kê cao/sàn và tác động tay. Lưu log từng bài riêng để so sánh với xe breadboard cùng firmware. Không đổi PID đồng thời với thay PCB.

### A. Chưa cấp điện

1. Kiểm đường khoan/ăn mòn/hàn, đặc biệt via nối hai mặt và chỗ pad sát nhau. Bản hiện tại không thể chế tạo một mặt rồi bỏ các via.
2. Đo thông mạch J1+→VM/IN+, OUT+→5V các bo/VCC driver, STM3V3→BMI/CS/encoder. Kiểm riêng PB6→GPIO18, PB7→GPIO17, PB8→SCL và PB9→SDA.
3. Kiểm không chập giữa pin3S/5V/3V3/GND, giữa UART TX/RX, I2C và ngõ motor. Tụ có thể làm số đo điện trở thay đổi khi đang nạp, không kết luận chỉ từ tiếng bíp ngắn.
4. Kiểm cực C1/C2/C4/C6, cực pin, vị trí module và GND chung. Xác nhận không có nguồn3S vào rail5V/3V3.

### B. Cấp nguồn từng phần, chưa nối motor

1. Thử buck trước khi gắn/cấp các module logic, đặt5,0V và kiểm đúng OUT+/OUT−. Ưu tiên nguồn bàn có giới hạn dòng phù hợp tải; không đoán giới hạn lớn hơn khả năng bo.
2. Cấp lần lượt STM, BMI và ESP, kiểm5V tại từng bo và3V3 tại BMI/CS. Ghi dòng khởi động, nhiệt bất thường và reset. Không đấu song song nguồn USB/ST-LINK cấp ra với nguồn đang nuôi bo khi chưa biết sơ đồ bảo vệ của devkit.
3. ESP mở Wi-Fi/web/truyền log liên tục; đo nguồn ngay tại chân ESP, không chỉ tại buck. Phải nằm trong dải làm việc của đúng module/regulator và không reset/brownout.

### C. Nền IMU và truyền thông, motor chưa chạy

Giữ xe đứng yên cố định, ghi log khoảng60s: accel/gyro/pitch, chu kỳ điều khiển, lỗi/mẫu mất I2C/UART/CRC và reset nếu firmware có trường này. Kiểm dữ liệu UART cả hai chiều. Nếu log không có trường cần đo, bổ sung logging trước khi kết luận, không coi trường vắng mặt là0lỗi.

### D. Motor kê cao, chassis được cố định

Thử riêng trái/phải, hai chiều, rồi hai bánh đồng thời, các mức PWM từ thấp lên cao; kiểm encoder đúng chiều/không đếm sai. Cần chế độ test motor trực tiếp đã giới hạn; nếu firmware hiện tại không có thì chuẩn bị firmware test riêng trước, không dùng chế độ cân bằng sàn để thay bài kê cao.

Đo VM tại driver,5V tại ESP/STM và3V3 tại BMI lúc khởi động, phanh và đảo chiều có kiểm soát. Tăng tải từng bước trong giới hạn motor/driver, không giữ kẹt bánh để lấy dòng stall. Theo dõi nhiệt đường đồng/via/đầu nối, reset, lỗi frame và mẫu encoder.

IMU tăng biên độ khi motor chạy có thể do rung cơ khí thật, không tự quy thành nhiễu điện. Đối chiếu đồng thời nguồn/dạng xung và điều kiện gá để phân biệt.

### E. Đo động bằng oscilloscope/logic analyzer

- Đo ripple/sụt áp ngay tại VM,5V ESP,5V STM,3V3 BMI; bắt sự kiện khởi động/phanh/Wi-Fi burst.
- Đo chênh GND driver–GND MCU bằng cách đo vi sai phù hợp; kiểm UART, I2C và encoder ngay tại đầu MCU khi motor chạy. Kiểm mức logic, ringing, xung giả và giải mã frame; không chỉ nhìn điện áp DC bằng đồng hồ.
- Kẹp mass oscilloscope thông thường chỉ vào GND hệ thống; không kẹp vào AOUT/BOUT/VM. Muốn đo chênh hai đầu motor hoặc hai điểm không cùng mass cần phương pháp/probe vi sai phù hợp.

Nếu chỉ có đồng hồ và log web, vẫn làm được A–D ở mức sàng lọc chức năng, nhưng không loại trừ được spike ngắn, ground bounce hoặc méo xung. Có thể mượn thiết bị đo để làm E trước khi chạy tải cao.

### F. Chạy sàn và so sánh với breadboard

Bắt đầu với firmware/gains đang ổn, khởi động gần thẳng và tốc độ thấp. Thu riêng log đứng cân bằng, tiến/lùi, trái/phải và dừng/thả nút; sau khi ổn mới tăng tốc và thử phục hồi. Quan sát trôi/xoay, reset, mất link, mẫu cảm biến/encoder bất thường. Lặp lại nhiều lượt ở cùng pin/sàn/firmware; chạy một lượt không đủ chứng nhận ổn định.

Chỉ coi bo đạt sau khi các rail không vượt dải module, không reset/lỗi truyền thông, encoder đúng, không quá nhiệt và hành vi xe ít nhất tương đương mốc breadboard trong các bài đối chiếu. Nếu có lỗi, sửa phần nguồn/tín hiệu trước khi dùng PID để che lỗi phần cứng.

## Nguồn nguyên tắc

- [TI — Best Practices for Board Layout of Motor Drivers](https://www.ti.com/lit/an/slva959b/slva959b.pdf): phân vùng công suất, đường hồi và tránh ghép nhiễu giữa đường chạy gần nhau.
- [Scilab/Xcos](https://help.scilab.org/docs/2025.1.0/en_US/xcos.html): khả năng mô phỏng phụ thuộc mô hình điện/hệ thống được xây dựng, không tự trích xuất PCB KiCad.
- Hướng dẫn đi dây của project là mốc pinout/nguồn; BOM_THT.md là mốc tụ hiện hành, thay các mô tả tụ cũ trong hướng dẫn.

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
