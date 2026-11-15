# RT10 MAX80 — phân tích log và bản vận hành

Ngày31/10/2026. Khối lượng theo hồ sơ hiện hành888g; nguồn người dùng khai báo12V, không có ADC pin/dòng trong log.

## Kết quả ba lượt duy nhất

File`twip_load_speed_R12 (2).txt` chứa lại lượt trong file(1). Đã tách theo epoch và thời gian STM32 để tránh đếm lặp. Chỉ xét các mẫu thực sự đang chạy thử, state3/fault0.

| Lượt/epoch | Số mẫu chạy | Đỉnh trung bình hai bánh | Trung bình500ms cao nhất | Góc CF lớn nhất khi chạy |
|---|---:|---:|---:|---:|
|1|224|89,956cm/s|86,465cm/s|7,443°|
|4|211|88,700cm/s|85,293cm/s|5,058°|
|7|218|88,573cm/s|85,390cm/s|6,846°|

Cả ba kết thúc reason7 (dự phòngPWM), không phải hết8giây hoặc đạt mục tiêu95cm/s. Trong các file chỉ thấy fault0 và fault1(lệnhSTOP); không thấy faultBMI160 trong ba file này. PWM tức thời có lúc tới2389/2499≈95,6%; bảo vệ cũ chờ≥90% liên tục100ms nên không phải trầnPWM90% tức thời.

Đỉnh là số đo encoder khi tăng tốc, chưa phải tốc độ giữ lâu ổn định hay tốc độ tiến thực đã đo độc lập. Bánh65mm: RPM=v×60/(π×0,065); các đỉnh khoảng260–264rpm, so với280rpm không tải. Không diễn giải tỷ lệRPM thành tỷ lệ công suất/mômen.

## Mức vận hành mới

Ba mức **50 /72 /MAX80cm/s**. MAX80 tương ứng khoảng235rpm, tăng11,1% so với72. Chọn80 thay vì dùng đỉnh89–90 làm mức chạy liên tục, vì cả ba lượt đã chạm dự phòngPWM khi còn tăng tốc. Chưa có log giữ80 lâu hoặc thử lùi80 thực tế; cần xác nhận bản mới trên xe.

Giữ R12/observer/phân bổ lực, hệ số motor, khối lượng/cơ khí và dốc tăng/giảm tốc. MAX80 sử dụng MOVE8 preset2 chính thức; preset0/1 vẫn50/72. Không còn lệnhLOADTEST hay tự chạy thẳng8giây. Người dùng giữ hướng/joystick để chạy và nhả để hãm.

Envelope MAX được giữ qua quá trình hãm/đổi mức để tránh chuyển ngay về ngưỡng0,8m/s khi xe chưa giảm tốc. MAX: trần trung bình0,95m/s, từng bánh1,0m/s. Predictor overspeed ởMAX chỉ xét trên0,90m/s; ở50/72 vẫn theo ngưỡng cũ0,78m/s. Ngưỡng tracking, góc, gyro, timeoutREC, va chạm và heartbeat/lease vẫn còn. Khi rờiMAX và vận tốc thực<0,76m/s, vref<0,73m/s, envelope cũ trở lại. Đây là giới hạn bảo vệ, không phải tốc độ đặt.

`motion_flags`:4096=đã chọnMAX80,8192=envelopeMAX đang giữ. Khác ý nghĩaLOADTEST cũ; dùng buildID để đọc đúng. Frame122byte/CSV40cột giữ nguyên.

## Dọn chức năng

- Bỏ bộ12bài/đo motor/điều khiển PWM thử trong STM32; bỏ nhánhPULSE/MEASURE/DIAG/PID/TRIM/RACE/BENCH khỏi bộ nhận lệnh. Chỉ nhậnARM/MOVE8/PING/STOP/CALIB.
- ESP32 bỏ trang hiệu chuẩn nâng cao, trang12bài, APIhardware/measurement, cấp phát2MB rawtest và3MB joblog, bộ nhận nhật ký thử và bảng thử trên web.
- Bỏ khốiLOADTEST khỏi web/STM32/gateway; preset2 nay là tốc độ vận hành80.
- Giữ bật/dừng cân bằng,8hướng,50/72/80, đồ thị trạng thái, đo lại gyro khi dừng, log vận hành, chẩn đoán kết nối và archive sau khởi động lại. Các mục này phục vụ vận hành/đọc lỗi thực tế.
- Tài liệu/kết quả thử cũ và công cụ kiểm thử trên máy là lịch sử phát triển, không phải chức năng chạy trên xe. Không xóa log gốc người dùng.

## Kiểm chứng và bản nạp

Các kiểm thử mới: `tools/test_production_controller.cjs`, `test_production_dashboard.cjs`, `test_production_session.cjs`, `test_production_gateway.cjs`. Có kiểm parser/FSM thật, từ chối lệnh thử, MAX80, hãm/đổi mức, owner/epoch/sequence, STOP, ARM và khóa tải log khi đang chạy. Thêm hồi quy `test_balance_recovery.cjs`, `test_rt8_tracking.cjs`; header luật/params R12 vẫn giống bản mô phỏng.

Mô phỏng chẩn đoán4ca,17giây/ca: tiến/lùi12V, tiến-quẹo trái12V, tiến-quẹo phải11,2V. Đều đạt vref0,8m/s, chạy đủ thời gian và về vref0 sau nhả; không vàoREC và không fault trong trace. Pitch thật lớn nhất khoảng7,02/7,03/7,04/8,03°. Kết quả `Simulation/results/runs/20261031_185831_526_motor_catalog_rt10_max80`. Mômen/ma sát/tiếp xúc của mô hình chưa được nhận dạng đầy đủ; đây không thay thế thử xe thật. Adapter chẩn đoán dùng cùng header động học/điều khiển, không mô phỏng Wi-Fi, I2C lỗi hoặc đầy đủ toàn bộFSM.

STM32 `BAL-20261031-R12-8DIR-RT10-MAX80`; ESP32 `TWIP-WEB-20261031-BAL-R12-8DIR-RT10-MAX80-NET7`. Nạp đồng bộ cả hai từ `.build/balance_release_r12_rt10_max80_20261031/`. Mở `http://192.168.4.1/ui/r12?v=rt10-max80-net7`, bật cân bằng rồi chọnMAX80 và giữ hướng. Thử50/72 trước để kiểm bản nạp và thao tác nhả/dừng, rồi thử80 trên đường đủ khoảng hãm.

STM32 sau dọn:text73724/data492/bss7444byte; LOAD2:text93440/data496/bss10384byte. ESP32 buildSDK3.3.11 thành công:1074083byte flash(34%),54004byte RAMtĩnh(16%); LOAD2 tương ứng1223443/59404byte. Ngoài giảm149360byte flash/5400byte RAMtĩnh còn bỏ cấp phát5MiB logthử trongPSRAM. Các header thử cũ ở `tools/legacy_factory/`, ngoài đường build firmware. Bộ kiểm thử cũ choLOADTEST/12bài đã được thay bằng bộproduction tương ứng vì ý nghĩa preset2 nay khác.

EnvelopeMAX chưa áp trong giai đoạn dựng xe/chu kỳARM đầu; giữ khả năng bắt cân bằng nền trước khi nhận vận hànhMAX. Chưa nạp hoặc xác nhận trên xe cho RT10. Dọn chức năng không được coi là sửa dứt điểm lỗiBMI160/mấtSTM32 sau ngã mạnh đã báo trước.
