> Cập nhật: đã gỡ bảng test encoder khỏi trang chính theo yêu cầu. ESP hiện hành **RT14-RELEASE-NET13**, STM vẫn RT14-RELEASE. Ticks/vận tốc/PWM vẫn được ghi trong log và bảng chi tiết thông thường.

# RT14 — log5 sau lắp lại dây,03/11/2026

Nguồn: Downloads/twip_balance_R12 (5).txt. Xe breadboard/WAGO không tụ ngoài; không quy lỗi cho thiếu tụ.

## Bằng chứng

Hai lượt state3 ghi được224,48s và36,96s, đều kết thúc fault1 có sự kiện STOP từ nút web, không phải tự ngã theo log. CRC/mấtframe/tràn/resetSTM được báo đều0. Vòng điều khiển4987–5012µs, thực thi lớn nhất647µs. Khi drive_sequence đổi, tuổi gói mới tại mẫu telemetry trung vị10ms, tối đa20ms. Đây không phải phép đo từ lúc ngón tay chạm màn hình quaWiFi.

Sau sửa cực trái, lượt tiến205,506–210,846s có vận tốc trung bình L/R0,232/0,231m/s; lùi294,646–298,206s là−0,491/−0,495m/s. Không còn quan hệ đảo dấu kéo dài của bánh trái nhưlog2/3. Có đủ lệnh tiến/lùi/quay và chéo. Không suy ra không có lực tay từ dữ liệu này.

Pha neutral298,206–298,666s vẫn có tốc độ trung bình khoảng−0,88m/s, vref−0,765m/s. Motor còn quay không đồng nghĩa lệnh joystick còn bị giữ: cần xem pha phanh, tốc độ và tham chiếu. Cờ16384 biểu thị giới hạn dự trữ góc/gia tốc, không phải lỗi mạng. Log không đoVM/dòng/tiếp xúc.

## Sửa có giới hạn

BalanceGovernor_Step có trường hợp thả nút nhưng gia tốc lưu từ pha tăng tốc tiếp tục làm tăng |vref| trong lúc giảm gia tốc theo jerk. RT14 hủy riêng gia tốc đang đẩy xa0 khi wanted=0, và chặn |vref_mới|>|vref_cũ| ở bước neutral. Có hồi quy cả hai chiều. Không khẳng định đây là nguyên nhân duy nhất của mọi cảm giác giật sau lắp lại.

Giữ nguyên CAT_K, observer, bộ phân bổ lực, dấu encoder/IMU, tốc độ50/72/95 và giới hạn bảo vệ. Mã cân bằng R12 không đổi; phần tạo tham chiếu khi thả nút được sửa. ESP cập nhật chuỗi tương thích STM và giải thích log, giữ mục encoder. Cần nạp cả hai.

STM: BAL-20261103-R12-8DIR-RT14-RELEASE. ESP: TWIP-WEB-20261103-BAL-R12-8DIR-RT14-RELEASE-NET12-ENC.

## Đấu dây hiện hành

Motor trái **đỏM1→AOUT2,trắngM2→AOUT1**. Phải đỏ→BOUT1,trắng→BOUT2. PA8→AIN1,PA9→AIN2 và encoder giữ nguyên. Bảng đỏ→AOUT1 trước đây không phản ánh đúng cực tính thực đã xác nhận; màu dây không chứng minh chiều bánh trong mọi cách lắp.

Schematic đổi hai rail tại U4. J2.1 vẫnM1đỏ,J2.6 vẫnM2trắng. PCB một lớp sửaW1/W2 và net tương ứng, không đổi kích thước/vị trí module hoặc lỗ. Vẫn33đoạn dây cầu. Dùng CSV/preview mới. Các Markdown đang dùng có ghi chú cấu hình mới; hướng dẫn Downloads cũng đồng bộ. Archive hai lớp giữ nguyên làm lịch sử, không dùng để gia công bản hiện tại.

## Vì sao tháo/lắp lại khác

Firmware đã hiệu chuẩn phụ thuộc dấu motor/encoder, hướngIMU và động lực học thực. Đảo dây trái khôi phục một sai khác đã có bằng chứng. Tiếp xúc breadboard, nguồn, độ siết trục, ma sát, góc cảm biến hoặc vị trí dây/pin có thể thay đổi sau lắp; log chưa phân biệt các yếu tố này. Không kết luận tất cả do phần cứng hoặc tất cả do code.

NạpRT14 rồi thử riêng đứng yên, tiến/thả,lùi/thả,quay hai chiều ở50cm/s trước; lưu từnglog và mô tả biểu hiện. Chưa thửMAX/đẩy mạnh ngay. Nếu còn giật, đoVM/5V tại tải; không chỉnhPID để che tiếp xúc. Bản mới cần kiểm chứng, chưa bảo đảm giống hệt xe trước tháo.
