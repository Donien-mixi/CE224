# Thử tốc độ cao nhất khi vẫn tự cân bằng — LOAD1

Chế độ tạm thời theo lựa chọn của người dùng ngày31/10/2026. Sau khi nhận log và kết thúc đợt thử, có thể gỡ chế độ này. Chưa nạp và chưa thử trên xe thật trong lượt triển khai.

## Khối lượng và nguồn

- Tổng trong mô hình hiện hành: **888 g** = thân788 g (đã gồm hai motor294 g) + bánh/khớp100 g. Nguồn: bảng `lk` và phép cộng trong `Simulation/model/robot_can_bang_pid_1.sce`. Đây là tổng theo hồ sơ linh kiện/bố trí; chưa cân lại cả xe trong phiên này.
- Nguồn người dùng khai báo **12 V**. Firmware chưa đo điện áp/dòng pin; trường12V trong log là thông tin người vận hành, không phải ADC. Hệ số điện R12 vẫn theo nền11,7V đã đóng băng.
- Bánh đường kính65mm: `rpm = v_mps * 60 / (pi * .065)`. 72cm/s tương ứng khoảng211,6rpm; 280rpm tương ứng95,3cm/s nếu lăn không trượt. Tỉ lệRPM không phải phần trăm công suất hay mômen motor.

## Hành vi

1. Bật cân bằng như bình thường. Xe chưa nhận lệnh chạy thử thì tiếp tục giữ cân bằng tại chỗ.
2. Bắt đầu thử chỉ khi đúng phiên điều khiển, dữ liệu còn mới, R12 sẵn sàng, đã hết khởi động/phục hồi, còn đủ chỗ lưu log. STM32 kiểm thêm200chu kỳ liên tiếp (1giây): góc≤3°, gyro<25°/s, mỗi bánh<0,05m/s, điểm đặt vận tốc<0,01m/s và không giữ hướng trước đó.
3. Chạy **thẳng tới trước**, yaw đặt0, vẫn đi qua bộ phản hồi/observer/phân bổ lực R12. Điểm đặt tăng theo dốc cũ:0,30m/s² dưới0,50m/s,0,15m/s² phía trên; mục tiêu trên0,95m/s. Không ghi PWM100% trực tiếp.
4. Bắt đầu hãm khi một trong các điều kiện xảy ra: nhả lệnh/mất gia hạn350ms; hết8giây; đi6m tính từ lúc bắt đầu; vào phục hồi/cancel; góc>10°; gyro>100°/s; hai bánh chênh>0,30m/s; hoặc **một motor đạt≥2249/2499 PWM liên tiếp20chu kỳ (100ms, xấp xỉ90%)**. Ngưỡng PWM giữ phần khả năng chỉnh cân bằng; không phải phép đo công suất còn lại.
5. Hãm theo dốc trung tính hiện có0,60m/s², vẫn giữ cân bằng. Trong chạy/hãm bài thử, trần trung bình1,05m/s và từng bánh1,10m/s; vượt trần hoặc gặp lỗi cứng thì cắt motor như nền hiện hành. Ngân sách hãm1m/4giây và các bảo vệ góc/va chạm/tracking/timing/IMU vẫn còn. Chạy thường50/72cm/s giữ nguyên.
6. Một lượt thử cho mỗi lần bật cân bằng; hết lượt không tự chạy tiếp khi web còn gửi hoặc kết nối lại. Đóng/ẩn trang làm mất lệnh thử; nếu heartbeat mất lâu hơn, bảo vệ nền có thể cắt motor. Web mất kết nối không phải cơ chế điều khiển từ xa đảm bảo tức thời.

Kết quả là **tốc độ cao nhất quan sát trong bài thử có giới hạn cân bằng/dự phòng**, không mặc nhiên là RPM tối đa tuyệt đối của motor khi có tải. Nếu kết thúc vì dự phòng, bảo vệ, thời gian hoặc quãng đường trước khi vận tốc ổn định, phải ghi rõ giới hạn đó. Bánh trượt làm tốc độ encoder khác tốc độ tiến thực.

## Cách thử và lấy log

Nạp **cả hai** bản tương ứng: STM32 `BAL-20261031-R12-8DIR-RT9-LOAD1`; ESP32 `TWIP-WEB-20261031-BAL-R12-8DIR-RT9-LOAD1-NET6`. Bản build tại `.build/balance_release_r12_rt9_load1_20261031/`; STM32 có ELF/HEX/BIN, ESP32 nằm trong thư mục `esp32`. Không dùng lại fileRT8 cũ cho một trong hai MCU.

1. Kết nối Wi-Fi xe, mở `http://192.168.4.1/ui/r12?v=rt9-load1-net6`, tải lại trang để nhận giao diện mới.
2. Dừng motor, chờ hơn2giây, tải log cũ nếu cần rồi xóa log để dành chỗ cho lượt mới.
3. Chuẩn bị đường thẳng trống **ít nhất8m**, gồm khoảng hãm; người giữ nút dừng đứng ngoài hướng xe chạy. Bật cân bằng trên nền, đợi xe ổn định, không giữ nút tiến/quay.
4. Mở **Thử tốc độ có tải · tạm thời**, đánh dấu đường thử và bấm **BẮT ĐẦU THỬ TỐC ĐỘ**. Chỉ coi bài thử đã bắt đầu khi STM32 xác nhận; web hiển thị vận tốc/RPM hai bánh và đỉnh quan sát.
5. Dùng **KẾT THÚC THỬ · HÃM VỀ ĐỨNG YÊN** để kết thúc sớm. Nút **DỪNG** chung cắt motor. Không chụp/giữ bánh đang được cấp lực; kết thúc bằng nút rồi mới giữ xe khi cần.
6. Khi xe đã hãm đứng yên, bấm **DỪNG**, chờ hơn2giây rồi tải log `twip_load_speed_R12.txt`. Gửi toàn bộ file; không cần chạy lại để làm số đỉnh đẹp hơn.

## Dấu hiệu trong log và cách gỡ

Frame UART122byte/CSV40cột giữ nguyên. `motion_flags`:4096 chạy thử,8192 hãm thử,16384 đã thực hiện/từ chối thử. Không nhầm các bit này với `fault` có cùng giá trị.

`$LTEST,phase,reason,start_ms,origin_m,operator_supply_V`: phase0chưa thử/1chạy/2hãm/3xong/4từ chối; reason1nhả hoặc hết lease/2thời gian/3quãng đường/4phục hồi hoặc bất ổn/5STOP hoặc lỗi/6chưa sẵn sàng/7dự phòng PWM. Metadata cuối chỉ mô tả lượt gần nhất; nên xuất mỗi lượt riêng.

Lệnh thử riêng `$LOADTEST,epoch,sequence,1*`, cùng chủ phiên/epoch/thứ tự/heartbeat với MOVE8. MOVE8 thường vẫn từ chối preset2. Khi gỡ: bỏ hai bản `load_speed_test.h`, nhánhLOADTEST ở hai bộ nhận lệnh, trạng thái/guard/metadata trong `robot_fsm.c`, phầnUI tạm và metadata/export ESP; đưa macro vận tốc về hai preset0/1. Đổi buildID đồng bộ và chạy lại kiểm thử, không xóa các sửaRT8 hoặc thay thông số R12.

## Kiểm chứng phần mềm

- 10 bộ CJS qua: `test_load_speed`, `test_drive8_controller`, `test_drive8_dashboard`, `test_drive8_session`, `test_drive8_gateway`, `test_net1_routes`, `test_balance_recovery`, `test_rt8_tracking`, `test_rt8_replay`, `test_rt8_ab` trong `tools/`.
- Header luật R12/params vẫn khớp bản mô phỏng; header frame hai MCU vẫn giống nhau. Kiểm qua parser/FSM thật với phần cứng giả lập, bao gồm lease, STOP, reset lượt khi bật lại và ngưỡng thử không bị REC nới ra.
- `node tools/run_load_speed_sim.cjs --run`: ba ca mô hình kín17giây, nguồn12V, nguồn10,8V và nguồn11,7V mất lệnh ởgiây8; cả ba bắt đầu, hãm và về điểm đặt vận tốc0. Ca12V kết thúc dự phòng ởgiây8,985 (3,985giây từ bắt đầu); ca10,8V ởgiây8,350; ca mất lệnh hãm ởgiây8,310. Kết quả cuối: `Simulation/results/runs/20261031_152029_994_motor_catalog_rt9_load1_feasibility`.
- Trong mô hình, đỉnh encoder khi còn chạy thử lần lượt khoảng0,848/0,755/0,744m/s. Đây **không phải dự báo RPM chắc chắn của xe thật**: mômen có tải, ma sát/tiếp xúc và dây breadboard chưa được nhận dạng/kiểm chứng đầy đủ. Chưa có log thửLOAD1 thực tế.
- Lỗi treo BMI160/UART sau ngã mạnh đã báo trước vẫn cần xử lý/kiểm chứng riêng; LOAD1 không được coi là bản sửa dứt điểm lỗi đó. Xem `Danh_Gia_Do_Ben_RT8_BMI160_20261031.md`.
- Kiểm thêm script cũ `test_hardware_boot.cjs`: phần chẩn đoánSTM32 qua, phần gateway host không biên dịch vì thiếu `esp_system.h` (mock cũ chưa đáp ứng phần network/ESP SDK). Không tính script này vào10bộ qua; kiểm gateway dùng `test_drive8_gateway`, `test_net1_routes` và buildESP32 vớiSDK thật.
- Build cuối cảSTM32/ESP32 thành công. STM32:text93408/data496/bss10384byte; ESP32:flash1223491byte(38%), RAMtĩnh59404byte(18%), SDK3.3.11, PSRAMopi/flash16MB/CDC. Đã kiểm buildID trong haiBIN và giao diện/lệnh nhả cuối trongBINESP; SHA-256 cùng dấu nguồnLF nằm ở `.build/balance_release_r12_rt9_load1_20261031/manifest.json`. ESP32 build cuối dùng `--jobs 1` sau lỗi thiếu bộ nhớ máy tính khi chạy song song; chưa nạp phần cứng.
