# RT17 — log (12) (2): bấm tiến nhưng xe lùi và bị hủy lệnh

Ngày kiểm tra: 03/11/2026. File nguồn: `C:/Users/DONG NHIEN/Downloads/twip_balance_R12 (12) (2).txt`. Firmware thật trong header: STM32 `RT17-SYNC`, ESP32 `NET16`. Người dùng xác nhận các lần bấm tiến nhưng xe lùi xảy ra trên **sàn trống, không chạm vật và không tác động tay**.

Phạm vi phiên này là đọc log, kiểm tra code và tái hiện nguyên nhân. **Chưa sửa firmware điều khiển, encoder, dấu IMU, đấu dây hoặc tạo bản RT18 để nạp.** Không coi các bài kiểm thử RT17 trước đó là bằng chứng đã giải quyết tình huống mới này.

## Kết luận chính

Log xác nhận xe có lùi thật trong một số thao tác tiến, không chỉ là cảm giác hoặc web hiển thị sai. Tuy nhiên, chưa có bằng chứng driver encoder đảo dấu hoặc đọc sai số học. Lỗi rõ ràng nằm ở sự phối hợp giữa quá trình tạo độ nghiêng để chạy, bộ giới hạn tham chiếu, phát hiện kẹt và điều kiện mở cổng điều khiển.

Chuỗi được quan sát:

1. Lệnh tiến đã tới STM32: `drive_direction=+1`, sequence tiếp tục tăng, tham chiếu vận tốc dương.
2. R12 có thể ra PWM âm ban đầu để tạo tư thế nghiêng tiến. Không được ép PWM dương chỉ vì joystick hướng tiến; như vậy sẽ can thiệp trực tiếp vào vòng cân bằng.
3. Khi vận tốc bánh đang âm mà tham chiếu dương, chênh lệch vượt 0,12 m/s. Bộ governor ghìm tham chiếu dù PWM còn nhỏ, góc chưa lớn và motor chưa gần bão hòa.
4. Xe bắt đầu quay lại chiều tiến nhưng tích lũy sai lệch vị trí hơn 6 cm; vận tốc còn dưới 5 cm/s. Bộ phát hiện kẹt đếm đủ khoảng 300 ms và hủy lệnh.
5. Giao diện thấy cổng không sẵn sàng nên gửi trung tính. Việc thả lệnh ở đây có thể đến từ logic giao diện, không thể từ CSV khẳng định người dùng đã tự nhả joystick.

Trên sàn trống như người dùng xác nhận, các lần này là nhận định kẹt sai. Bộ phát hiện đang nhầm một phần quá độ khởi hành/đảo chiều với xe tì vật cản.

## Toàn bộ lượt chạy

Có 5.783 mẫu, trong đó 5.385 mẫu state3; log không tràn, CRC=0, sequence_gaps=0, STM32_resets=0. Metadata cuối ghi các bộ đếm lỗi I2C bằng0. Không có fault32/64 trong các lượt này. Log 50 Hz không quan sát mọi chu kỳ điều khiển 200 Hz hoặc thời điểm ngón tay chạm màn hình.

| Bắt đầu STM32 | Thời gian state3 | Kết thúc | Đặc điểm |
|---:|---:|---|---|
| 44,767 s | 0,44 s | Fault4, góc vượt giới hạn | Khởi động khoảng −23,15°, chưa có lệnh chạy |
| 49,647 s | 4,98 s | STOP từ web | Cân bằng, mở cổng điều khiển, chưa chạy |
| 69,347 s | 57,92 s | STOP từ web | Không mở cổng điều khiển dù đã cân bằng |
| 133,447 s | 44,28 s | STOP từ web | Có điều khiển, 7 lần tiến bị gán kẹt |

Lượt đầu là một lần dựng không thành công riêng, không phải hậu quả joystick tiến bị đảo thành lùi. Ba lượt sau dừng bằng nút STOP theo bằng chứng GSTOP và fault1.

## Bằng chứng lệnh tiến đến nơi nhưng xe lùi

Ví dụ ở **148,187 s**:

- Hướng đã giải mã: tiến `+1`, không quay.
- Tham chiếu: khoảng **+4,7 cm/s**.
- Tốc độ trung bình encoder: khoảng **−8,1 cm/s**.
- Chênh lệch: khoảng **12,7 cm/s**, lớn hơn ngưỡng governor 12 cm/s.
- Pitch khoảng **+1,67°**, gyro khoảng **+6,6°/s**.
- PWM trái/phải **−356/−334**, còn xa giới hạn ±2499.
- Cờ16384 đang bật; lệnh sequence605 được STM32 nhận khoảng10ms trước mẫu này. Con số này là tuổi của bản tin mới nhất, không phải tổng độ trễ từ ngón tay tới bánh.

Đến **148,687 s**, tốc độ đã thành **+3,3 cm/s**, tham chiếu khoảng+7,3 cm/s, nhưng sai lệch vị trí khoảng7,3cm. Tại **148,807 s**, bộ phát hiện kẹt hủy thao tác, cờ128 bật và tham chiếu bị chuyển sang phanh. Tình trạng tương tự lặp ở các mốc sau.

| Thời điểm hủy | Sai lệch vị trí ngay trước hủy |
|---:|---:|
|147,187 s|6,25 cm|
|148,807 s|7,96 cm|
|150,527 s|9,26 cm|
|155,607 s|8,01 cm|
|161,507 s|8,32 cm|
|164,287 s|7,70 cm|
|165,867 s|7,75 cm|

Cả7 lần đều đang yêu cầu tiến; không có REC1 ở các thời điểm này. Không nên giải thích các lần hủy đó bằng bảo vệ va chạm REC1, lỗi đọc BMI160 hoặc Wi-Fi mất kết nối.

Vẫn có thao tác tiến thành công: đoạn138,327–141,347s tăng tới khoảng50cm/s, tiến khoảng75,5cm theo encoder. Đoạn170,987–173,427s cũng tiến được khoảng48,1cm. Do đó không phù hợp với giả thuyết hướng joystick hoặc encoder bị đảo dấu cố định.

## Encoder hiện vận hành như thế nào

`encoder.c` dùng TIM2 cho PA0/PA1 bánh trái và TIM3 cho PB4/PB5 bánh phải. Cả hai cấu hình TI12, chu kỳ65535, lọc đầu vào6. Theo [ST RM0383](https://www.st.com/resource/en/reference_manual/rm0383-stm32f411xce-advanced-armbased-32bit-mcus-stmicroelectronics.pdf), timer encoder suy ra chiều và cập nhật bộ đếm từ quan hệ pha hai tín hiệu; đặt CounterMode=UP trong cấu hình không ép encoder chỉ đếm tăng.

- Chênh lệch mỗi5ms được ép `int16_t` để xử lý vòng65535→0 và ngược lại.
- Trái giữ dấu; phải đảo dấu để vận tốc xử lý của cả hai cùng dương khi tiến.
- `Encoder_GetHardwareTicks` công bố lại chiều thô: trái tăng, phải giảm khi tiến. Cột ticks phải âm **không đồng nghĩa** vận tốc phải bị lỗi.
- Quãng đường dùng `0.5*(ticks_l-ticks_r)*METERS_PER_TICK`.
- PPR898,225, bánh65mm, mỗi tick≈0,22734mm. LPF beta0,75 ở5ms có trễ trung bình khoảng15ms, không giải thích được các lần bị hủy kéo dài gần1s.
- Reset chỉ ở init, hiệu chuẩn khi đã dừng và ARM. Không reset khi bấm/nhả joystick hay REC1.

Kiểm tra log:

- Quãng đường công bố khớp phép tính từ ticks, sai số lớn nhất khoảng0,000000534m, phù hợp làm tròn CSV/float.
- Tích phân vận tốc50Hz trong cửa sổ200ms, có bù trễ LPF, lệch quãng đường ticks trung vị0,107mm; phân vị99 khoảng0,525mm. Đây là phép kiểm tra nhất quán, không phải hiệu chuẩn độc lập số xung/vòng.
- Bước tick lớn nhất giữa hai mẫu20ms đang chạy: trái75, phải92. Không thấy cú nhảy hàng chục nghìn tick gợi ý lỗi tràn số.
- Chạy lại kiểm thử driver encoder thật với timer giả lập: cả hai chiều, qua vòng đếm, kênh độc lập và dừng yên đều đạt.

Những kiểm tra này không loại trừ hoàn toàn nhiễu điện, mất xung thật, sai PPR cơ học hoặc trượt bánh; cùng một nguồn ticks được dùng để tính vận tốc và vị trí. Tuy nhiên, log hiện không ủng hộ việc đảo dấu encoder hay đổi dây A/B để sửa lỗi này.

## Vì sao có lượt gần58 giây không nhận điều khiển

Sau pha dựng, mốc `x_ref` vẫn0. Xe tự đứng ở vị trí khoảng−8 đến−14cm so với lúc ARM. `BalanceMotion_Update` đòi `abs(position-x_ref)<0.08` để tích lũy500ms xác nhận ready. Có2.816 mẫu đã thỏa các điều kiện góc, gyro, tốc độ và tham chiếu yên nhưng bị chặn vì sai lệch vị trí.

Hệ quả: gần cả lượt69,347–127,267s không có ready, sequence0, direction0, dù xe vẫn cân bằng. Đây là **cổng không cho phát lệnh**, không phải encoder bỏ lệnh joystick. Nút ARM có readiness riêng; cột `ready` chung và bit1 trong `motion_flags` không cùng ý nghĩa.

## Tái hiện với đúng header C

Đã biên dịch thử trực tiếp `balance_governor.h`, `balance_motion.h`, `balance_turn.h`, không thay driver hoặc firmware:

1. Xe yên ở−11cm, pitch2,8°, tốc độ0, neutral liên tục10s: cổng điều khiển vẫn không mở vì mốc0 và ngưỡng8cm.
2. Tham chiếu+2,1cm/s, tốc độ−10,2cm/s, duty20%, pitch1,74°, gyro8,7°/s: governor vào hạn chế bởi sai lệch0,123m/s, dù dự báo góc chỉ2,61°.
3. Thao tác tiến đang chạy, sai lệch vị trí8cm, tốc độ+2,6cm/s: sau300ms điều kiện kẹt vẫn hủy lệnh dù không có lỗi IMU. Đây là thử logic với trạng thái cố định, không phải mô phỏng mới của chuyển động xe.

Kết quả: `output/RT17_Log12/reproduction.txt`. Biểu đồ: `output/RT17_Log12/chan_doan_lenh_tien.png`. Bảng số và trích mẫu: `output/RT17_Log12/analysis.json`.

## Hướng sửa cần kiểm chứng tiếp

1. Tách cổng ready sau dựng khỏi yêu cầu phải quay lại đúng vị trí ARM. Việc chốt/slew mốc dừng mới phải có điều kiện yên và giới hạn để không tạo bước lực hoặc liên tục bám theo bánh làm mất chống trôi.
2. Bộ governor cần phân biệt quá độ tạo nghiêng/đổi chiều với thiếu dự trữ kéo thật. Không chỉ bỏ ngưỡng0,12m/s hoặc nới góc hàng loạt; tiếp tục giữ duty, góc/gyro, giới hạn vật lý và thời gian.
3. Bộ phát hiện kẹt cần xét giai đoạn khởi hành, hướng và mức vận tốc tham chiếu, tiến triển thật và bằng chứng nỗ lực motor. Điểm đặt nhỏ + vận tốc đang đổi dấu không đủ để khẳng định kẹt. Vẫn phải chặn tình huống tì tường thật, không tự phát lại thao tác đã nhả và không kéo dài vô hạn episode.
4. Thêm đúng ca hồi quy từ log: neutral đứng lệch11cm sau dựng; bấm tiến từ tư thế nghỉ2–4°; tốc độ còn lùi khoảng10cm/s; chuỗi nhấn/nhả dưới1s; vật cản thật; hai hướng đối xứng; VM11,2/11,7V. Các ca RT17 trước đây đã bỏ sót sự tương tác này.

Không đổi dấu encoder, màu dây motor hoặc bộ gain R12 để che các lỗi trên. Chưa có bản firmware sửa tiếp được xác nhận trong phiên chẩn đoán này; nạp lại cùng RT17 không làm lỗi tự hết.
