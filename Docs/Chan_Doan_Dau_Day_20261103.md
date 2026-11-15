> CẤU HÌNH ĐẤU DÂY ĐÃ XÁC NHẬN 03/11/2026: **motor trái đỏ(M1) → AOUT2, trắng(M2) → AOUT1**; motor phải đỏ → BOUT1, trắng → BOUT2. Encoder và PA8/PA9 không đảo. Bảng màu trái ngược trong ghi chép lịch sử không dùng để lắp lại xe.

# Chẩn đoán sau tháo/lắp dây — 03/11/2026

Nguồn: hai file Downloads/twip_balance_R12 (2).txt và (3).txt; hướng dẫn đi dây đính kèm trong Downloads, ngày30/10. Phạm vi là breadboard/WAGO không tụ ngoài; không sửa firmware, PID hoặc mô hình xe.

## Kết luận ưu tiên

Log cho thấy quan hệ dấu PWM–chuyển động bánh trái ngược mong đợi, trong khi bánh phải cùng dấu đúng kỳ vọng. Ưu tiên kiểm đường PA8/PA9→AIN1/AIN2 và AOUT1/AOUT2→motor trái; phân biệt với encoder trái A/B đảo bằng bài quay tay. Chưa đủ bằng chứng kết luận hỏng motor/driver hoặc thiếu áp; không đổi đồng thời dây motor và encoder.

## Bằng chứng

| Nội dung | Log(2) | Log(3) |
|---|---:|---:|
| Lượt state3 ghi được |76,06s|13,46s|
| PWM tuyệt đối trung bình trái/phải |294,35/268,12|288,13/654,19|
| Tốc độ tuyệt đối trung bình trái/phải |0,0164/0,0531m/s|0,0471/0,1605m/s|
| Nguyên nhân dừng |fault4, vượt góc|fault4, vượt góc|
| Pitch ở mẫu đã dừng |+52,83°|−60,93°|

Các lượt có CRC_errors=0, sequence_gaps=0, overflow=0, STM32_resets=0 theo header. Build được khai báo là BAL-20261101-R12-8DIR-RT13-RESP95. Không có lệnh tiến/lùi/quay trong các mẫu cân bằng: drive_direction=turn_direction=0. Header không chứng minh đồng nhất binary hoặc đủ nguồn dưới tải; log không đo điện áp/dòng.

Ví dụ cửa sổ0,5s từ khoảng1s sau bắt đầu log(2): PWM trung bình L/R=−330/−377, vận tốc L/R=+0,0134/−0,0634m/s. Log(3), cửa sổ0,5s tại3s: PWM=−277/−280, vận tốc=+0,0430/−0,0495m/s. Cùng lệnh dấu âm nhưng bánh trái báo chuyển động dấu dương.

Trong1532cửa sổ chồng lấn100ms của log(2), PWM trái giữ âm với điểm đầu dưới−300 và các mẫu tiếp theo dưới−100; cả1532đều có delta_ticks trái dương. Log(3) tương tự203/203cửa sổ. Các cửa sổ không phải1532/203thử nghiệm độc lập. Bánh phải phần lớn đổi dấu đúng theo PWM. Ở log(2) còn15cửa sổ trái PWM dương ổn định; delta_ticks trái đều âm. Quan hệ đảo dấu xuất hiện cả hai chiều trong dữ liệu, không chỉ một mẫu tức thời khi đảo lệnh.

Kiểm bổ trợ: gyroZ sau trừ bias so với (vR−vL)/0,262 có tương quan0,980/0,997 ở các mẫu lọc |pitch|<10°, |gyroY|<40°/s, normaccel0,9–1,15g. Với giả thuyết đảo riêng dấu encoder trái, tương quan giảm và RMSE tăng. Điều này ủng hộ chuyển động quay thật và ưu tiên lỗi chiều tác động motor trái hơn lỗi dấu encoder đơn thuần, nhưng không thay thế xác minh từng bánh ngoài thực tế; trượt bánh, lực tay và hướng cảm biến có thể ảnh hưởng.

## Tài liệu đi dây gửi kèm

Bản Downloads còn bảng5.2 cũ, dùng thứ tự chân biến thể driver khác và thiếu2chân nguồn lặp. Không dùng vị trí hàng trong bản này để cắm theo ảnh module người dùng. Theo bản ánh xạ ảnh đã lưu:

- Hàng trên của ảnh: GND,VM,VCC,BIN2,BIN1,AOUT1,AOUT2.
- Hàng dưới: BOUT2,BOUT1,AIN1,AIN2,VCC,VM,GND.

Đọc nhãn in thực tế là ưu tiên. Firmware dùng PA8→AIN1,PA9→AIN2; PA10→BIN1,PA11→BIN2. Encoder trái PA0/PA1, encoder phải PB4/PB5, phải được đảo dấu trong phần mềm. Quy ước màu đỏ/trắng trong tài liệu chưa phải phép đo xác nhận cực tính cơ học sau tháo lắp.

## Quy trình phân biệt lỗi, không sửa gain

1. Tắt pin; đánh dấu/truy dây từ chân STM32 đến nhãn AIN/BIN thực, rồi từ AOUT/BOUT đến bánh thực. Nếu bốn dây input sai thì sửa đúng nhãn trước, không bù bằng đổi gain.
2. Tắt pin và tháo riêng dây công suất đỏ/trắng khỏi driver; giữ encoder. Cấp logic, quay tay đỉnh bánh về WAGO nguồn pin. Chỉ bánh trái quay thì chỉ ticks trái thay đổi và tăng; chỉ bánh phải quay thì chỉ ticks phải thay đổi và giảm. So biến thiên ticks, không chỉ giá trị tuyệt đối. Vận tốc đã xử lý của cả hai khi tiến phải dương.
3. Nếu bước2 sai, sửa dây encoder đúng bánh/thứ tự hoặc tiếp xúc; chưa đảo motor. Nếu bước2 đạt, input PA8/PA9 đúng nhưng khi thử motor riêng PWM dương bánh trái lại lùi thì đảo một cặp dây công suất trái AOUT1/AOUT2. Không đồng thời đảo A/B encoder; tiêu chí cuối là PWM dương→bánh tiến→v đã xử lý dương.
4. Source hiện hành không mở lệnh thử bánh riêng trên web; cần bài chẩn đoán motor có giới hạn trước khi làm bước3 bằng lệnh motor. Không dùng ARM cân bằng khi kê cao để thay bài pulse; không đấu trực tiếp pin để thử tùy tiện.
5. Khi dấu từng bánh đã đúng mà trái vẫn yếu dưới cùng PWM/tải, mới đo VM tại driver và điện áp giữa OUT1/OUT2 từng kênh; kiểm tiếp xúc nguồn, chân input PWM, cơ khí và driver. Không suy ra thiếu áp từ tốc độ khi đóng vòng.

Pitch điều khiển hiện tại hướng WAGO pin là dương, hướng WAGO5V là âm. Không xoay BMI160 hoặc đảo dấu pitch theo lời hướng dẫn trước đã nhầm góc thô và góc điều khiển.

Kết quả thô tái lập trong .build/rewire_20261103/analysis.json, analyze.py và yaw_check.py. Chỉ tạo hồ sơ phân tích; không đổi firmware hoặc PID.
