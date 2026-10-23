# Kiểm tra kết nối và mất log — RT4 NET1

STM32 giữ `BAL-20261022-R12-8DIR-RT4`. ESP32 dùng `TWIP-WEB-20261022-BAL-R12-8DIR-RT4-NET1`. Chỉ nạp ESP32 cho lần sửa này nếu STM32 đã là RT4; không xóa toàn bộ Flash.

1. Cấp pin, kết nối `TWIP_RACER_S3`, mở `http://192.168.4.1/ui/r12?v=rt4-net1`. iPhone: bật chế độ máy bay rồi bật lại Wi-Fi. Wi-Fi của xe không cung cấp Internet.
2. Đọc **Kết nối và log dự phòng**. Ghi lại số phiên ESP32 và lý do khởi động. `power_on` ở lần vừa cắm pin là bình thường. Nếu số phiên tự tăng trong lúc chạy thì ESP32 đã reset; `brownout` chỉ rõ detector báo nguồn thấp, `panic`/watchdog cần điều tra phần mềm. Mất nguồn hoàn toàn có thể hiện `power_on`, không loại trừ sự cố nguồn.
3. Để motor dừng khoảng1–2phút xem AP có ổn không. Sau đó thử bánh kê cao, rồi chạy trên nền ở mức thường. Không tăng tốc/độ khó khi mạng/nguồn chưa ổn. Giữ web trên màn hình, không mở camera trên cùng điện thoại điều khiển.
4. Sau khi dừng2s, tải log RAM như cũ. Đồng thời tải **TẢI CHẨN ĐOÁN KẾT NỐI**. Nếu ESP32 đã reset và RAM về0, kết nối lại, chọn **TẢI LOG FLASH** của phiên trước; tên file chứa số phiên/đợt xóa RAM.
5. Gửi TXT và JSON chẩn đoán để đối chiếu. File Flash chỉ gồm các batch CRC hợp lệ, có thể thiếu đuôi chưa ghi; không được coi là lượt thử đầy đủ nếu mất nguồn. Không cần tự sửa note hay đổi bộ cân bằng.

Flash giữ4file, không tự xóa. Tải/kiểm tra rồi xóa file cũ khi motor dừng. File đang gắn với log RAM chưa được xóa; sau khi đã tải, xóa log RAM trước rồi mới xóa bản Flash đó. Nếu báo mount lỗi, phần mềm giữ nguyên dữ liệu cũ; không tự format một phân vùng đã có dữ liệu.

Nếu reset gắn với motor chạy, kiểm tra nguồn trước khi đổi PID: đo5V ngay tại ESP32 và3V3 trên board khi khởi động/đảo chiều; đo tĩnh ở XL4015 chưa đủ phát hiện sụt ngắn. Kiểm tra tiếp xúc WAGO/dây cấp, dây nguồn ngắn/chắc, mass motor không đi qua đường hồi của ESP32, ăng-ten không bị kim loại che. Không cấp5V vào chân3V3, không đấu song song hai nguồn5V trực tiếp. Không tắt bảo vệ brownout/watchdog.

Giới hạn: chưa đo trực tiếp nguồn, RF hoặc xác nhận trên xe. NET1 giúp giảm tải và xác định reset, không thể sửa một đường nguồn đang sụt bằng thuật toán Wi-Fi. Tốc độ/PID/khởi động60°/giới hạn STM32 không đổi.

Tham khảo: [Apple về kết nối Internet/Wi-Fi](https://support.apple.com/en-ie/guide/iphone/iphd1cf4268/27/ios/27), [Espressif về reset reason](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/misc_system_api.html), [Arduino ESP32 Wi-Fi API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/wifi.html).
