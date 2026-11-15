> Bản PCB ngày 06/11/2026 dùng một mặt đồng B.Cu, 31 đoạn dây cầu trong 24 nhóm. Tâm thân BMI160 tại (130,00; 81,65) mm. Xem [SINGLE_LAYER.md](SINGLE_LAYER.md) và [bảng dây cầu](SINGLE_LAYER_WIRES.csv) cho bản lắp hiện hành. Các tọa độ bố trí và số dây của bản trước bên dưới là dữ liệu lịch sử.

# Đánh giá trước gia công — PCB TWIP

Ngày 02/11/2026. PCB SHA256: `3b45fc721898aa71efdf014fbb5e16eceaefa4611bad08ea62a22b4146117fcc`.

## Quyết định hiện tại

Đạt kiểm tra kết nối CAD theo các quy tắc đang cấu hình; chưa chốt khả năng chịu dòng, nhiễu và tương đương thực nghiệm với bản breadboard/WAGO. Nên xử lý các điểm ưu tiên bên dưới và làm bo mẫu trước khi đặt số lượng nhiều. Không thay layout hay firmware trong lần đánh giá này.

Phân nhánh nguồn giữ cùng chức năng với bản cũ, không có nghĩa điện trở, điện cảm, tụ ký sinh, đường hồi và cách chia nhánh vật lý hoàn toàn giống nhau. Bản cũ còn có ray breadboard dùng chung giữa một số tải. Chưa có chiều dài/tiết diện dây, điện trở tiếp điểm và số đo nguồn động của bản cũ để so sánh định lượng hai mạng.

## Những kiểm tra đã thực hiện

- KiCad ERC/DRC/parity trên cùng bản PCB: 0 vi phạm, 0 kết nối thiếu, 0 khác biệt schematic–PCB. Đối chiếu 80 dòng chân logic khớp net thực.
- DRC dùng quy tắc hiện có, không phải chứng nhận theo nhà gia công. Báo cáo có các nhóm bỏ qua: missing_courtyard, track_not_centered_on_via, tuning_profile_track_geometries, footprint_filters_mismatch, footprint_type_mismatch. Không coi 0 vi phạm là mọi loại kiểm tra đều đã bật.
- Đọc trực tiếp đoạn track/pad/via; đo khoảng hở tín hiệu–motor cùng lớp và giao hình chiếu ở hai lớp khác nhau.
- Dựng đồ thị đường đồng nguồn để ước tính điện trở DC một đường dẫn, giả định đồng dày35µm ở20°C. Không giải phân bố dòng trên plane, điện cảm hoặc nhiễu PWM.
- Mặt đổ GND F.Cu có4 polygon được đổ; B.Cu có1 polygon được đổ. Số polygon không chứng minh các vùng bị cô lập điện: chúng có thể nối qua pad/via/đường đồng. Một polygon cũng có thể có cổ hẹp và đường vòng. Chưa thực hiện trích xuất trường điện từ hoặc bản đồ mật độ dòng GND.

Kết quả tính mới nằm tại `.build/pcb/release_audit/geometry_audit.json` và `dc_paths.json`. Các số chiều dài trong VALIDATION_PLAN.md của layout cũ không dùng làm số hiện hành.

## Nguồn — ước tính DC có điều kiện

| Đường dẫn | Chiều dài track của đường chọn | R đồng ước tính | Sụt áp nếu riêng đường này mang0,5A |
|---|---:|---:|---:|
| XL4015 OUT+ → ESP32 5V |54,97mm|0,0363Ω|18,1mV|
| XL4015 OUT+ → một pad STM32 5V |101,26mm|0,0688Ω|34,4mV|
| XL4015 OUT+ → một pad driver VCC |54,66mm|0,0361Ω|18,0mV|
| J1 BAT+ → XL4015 IN+ |56,95mm|0,0187Ω|9,35mV|
| J1 BAT+ → một pad driver VM |35,69mm|0,0117Ω|5,86mV|

Đây là một đường trong đồ thị track, không phải tổng chiều dài mọi nhánh hoặc điện trở tương đương toàn mạng. Via/PTH được coi lý tưởng; bỏ qua điện trở tiếp xúc, pad, đường hồi GND, mạch trong module, nhiệt và tương tác dòng các tải trên đoạn chung. 0,5A là tình huống giả định để nhìn độ nhạy, không phải dòng tải đo. Các số này không chứng minh rail sạch hoặc đủ dòng. Cần chốt độ dày đồng thành phẩm với xưởng và tính lại nếu thay đổi.

## Các điểm cần ưu tiên trước khi chốt

### Đường motor

- MOTOR_L_M1: tổng đoạn track80,47mm, rộng1,5mm, không có via chuyển lớp.
- MOTOR_L_M2: tổng đoạn71,80mm, có các bề rộng0,9/1,125/1,5mm và1via đường kính đồng1mm/lỗ0,5mm.
- MOTOR_R_M1: tổng đoạn26,44mm, rộng1,5mm và1via đồng1mm/lỗ0,5mm.
- MOTOR_R_M2: tổng đoạn13,41mm, rộng1,5mm và1via đồng1mm/lỗ0,5mm.

Nên giảm chỗ thắt và giảm phụ thuộc một via đơn cho đường công suất bằng đổi tuyến hoặc via song song phù hợp khoảng hở. Chưa được kết luận các via/track này quá tải vì thiếu dòng và thông số mạ lỗ; cũng chưa thể chứng nhận đủ tải.

### Encoder, UART, I2C

| Net | Tổng chiều dài đoạn track | Khoảng mép track nhỏ nhất tới motor cùng lớp | Giao centerline hình chiếu motor khác lớp |
|---|---:|---:|---:|
| Encoder trái A |126,04mm|1,35mm|0|
| Encoder trái B |117,52mm|1,35mm|0|
| Encoder phải A |124,55mm|0,539mm|1|
| Encoder phải B |92,67mm|3,649mm|1|
| I2C SCL |67,68mm|11,60mm|0|
| I2C SDA |68,32mm|13,58mm|0|
| STM TX → ESP RX |125,59mm|27,96mm|0|
| ESP TX → STM RX |124,56mm|28,71mm|0|

Chỉ tính track, không gồm pad/via/plane/dây ngoài. Khoảng0,539mm không phải lỗi cách điện hoặc ngưỡng chắc chắn gây nhiễu. Ưu tiên xem đường ENC_R_A để tách khỏi đầu ra đóng cắt motor, tránh chạy gần song song và bảo đảm đường hồi tín hiệu. Không coi giao khác lớp là chập hoặc tự động thất bại EMC.

### GND và mô hình dòng

Giữ plane GND chung không tự động kém hơn star-ground. Cần ưu tiên vị trí driver, đường pin/motor và tụ VM để vòng dòng đóng cắt không đi vòng qua khối IMU/MCU. Không tùy tiện cắt mặt GND thành đảo hoặc buộc UART/I2C băng khe để tạo hình sao trên hình vẽ.

Nếu muốn bản đồ dòng/sụt áp GND bằng công cụ trường điện, cần bộ giải mạng nguồn/PEEC/FEM thích hợp, stackup và mạ lỗ, dòng tại các cổng theo thời gian, cách nối nội bộ module và dây ngoài. SPICE với RLC trích xuất có thể kiểm rail nguồn theo giả thiết; Scilab mô hình cơ khí/PID không thay thế phép này. Chưa chạy các bộ giải đó, không có ảnh bản đồ dòng giả định được gắn nhãn là phép đo.

## Điều kiện chốt đơn gia công

1. Chốt bản layout sau khi xử lý/đánh giá các mục trên; refill GND, kiểm ERC/DRC/parity lại. Rà riêng các vùng còn hẹp và đường hồi gần cảm biến.
2. Xác nhận chân module bằng đối chiếu thực, đúng tụ và đầu nối. Chốt dòng định mức J1; xem lại tải còi PB12 chưa có dữ liệu. Không coi lời xác nhận sơ đồ điện là chứng nhận những phần này.
3. Chốt vật liệu/thông số với xưởng: carrier hai lớp, FR-4 dày1,6mm theo CAD; độ dày đồng thành phẩm chọn cụ thể (35µm chỉ là giả định đang tính), mạ xuyên lỗ pad/via, solder mask hai mặt; lỗ M3 NPTH3,2mm theo hồ sơ. Xin DFM theo năng lực xưởng, đặc biệt mép lỗ M3 còn0,9mm và kích thước lỗ thành phẩm.
4. Xuất Gerber hai lớp đồng, mask, silk, Edge.Cuts, bộ drill PTH/NPTH và xem lại độc lập; dùng chúng để đặt mạch, không dùng PDF ướm linh kiện.
5. Đặt số lượng mẫu tối thiểu phù hợp dịch vụ trước. Thử theo LAP_DAT_VA_CAP_NGUON.md và VALIDATION_PLAN.md: nguồn không motor; Wi-Fi; từng motor; hai motor/phanh/đảo; cuối cùng cân bằng/chạy sàn, giữ firmware mốc.
6. Đo nguồn ngay tại tải, lỗi/reset và tín hiệu khi motor chạy. Nếu chỉ có đồng hồ/log, chỉ sàng lọc chức năng; muốn đánh giá xung ngắn cần oscilloscope và phương pháp đo phù hợp.

Không thể chứng nhận tuyệt đối từ laptop rằng bo mẫu sẽ luôn cân bằng và không nhiễu như bản cũ. Có thể giảm rủi ro có hệ thống trước gia công và xác nhận bo mẫu trước khi dùng làm bản cuối.

## Cơ sở kỹ thuật

- TI, Best Practices for Board Layout of Motor Drivers (SLVA959B): https://www.ti.com/lit/an/slva959b/slva959b.pdf — đường hồi, plane GND, phân vùng, via và tụ; nguyên tắc ở cấp IC không thay sơ đồ nội bộ các module.
- KiCad PCB Editor: https://docs.kicad.org/master/en/pcbnew/pcbnew.html — DRC, zone và đầu ra chế tạo; áp dụng theo bản KiCad10 đang dùng.
