> **Trạng thái:** bản hiện hành **STM32 RT34-CORNER + ESP32 NET35** (17/11/2026) — xem
> [tổng quan](Tong_Quan_Do_An.md) và [HANDOFF](../HANDOFF.md).

# Chức năng xe hiện hành — RT34-CORNER / NET35

Danh sách mô tả **đúng** những gì code đang chạy, kèm số đo từ log xe thật. Đây không phải chứng
nhận mọi tình huống thực tế đã đạt.

## 1. Ba mức tốc độ

| Mức | Tốc độ đặt | Ghi chú |
|---|---:|---|
| 0 – chậm | 0,50 m/s | mặc định |
| 1 – vừa | **0,80 m/s** | dùng khi pin 11,2–12,0 V |
| 2 – MAX | **0,90 m/s** | chỉ khi pin **≥ 12,0 V** (duty 0,879 @12,2 V; 0,955 @11,2 V) |

Đo từ log: xe đạt `v_ref` p95 đúng mốc đặt; đỉnh thực tế ~0,93 m/s (bứt tốc). Trần không tải đo được
**0,96 m/s** ở pack 11,7 V, nên 0,90 là mức sát trần — không tăng thêm được trên 3S.

## 2. Bẻ hướng 8 hướng

- Quay tại chỗ: 1,3 rad/s (mức chậm) và 2,4 rad/s (mức vừa/MAX) — đo được 2,43 rad/s ở cả hai chiều.
- Chéo: 0,90 / 1,15 rad/s, gia tốc yaw 4,5 rad/s², giảm tốc 8 rad/s², trần ngang 0,90 m/s².
- **Bốn hướng lái cân nhau** sau khi bù lệch bánh trái: lệch trái/phải còn **1 % (tiến), 2 % (lùi),
  0 % (quay tại chỗ)** — trước khi bù là 13 %.
- **Trần tốc độ do đường**: khi đang cua, tham chiếu tốc độ tự hạ xuống `0,95 − |w_ref|·0,131`, nên
  cua ở MAX 90 chạy ở ~0,80 m/s thay vì bão hoà bánh. Cờ 512 trong cua giảm từ 50–77 % xuống 19–26 %.
- Độ nghiêng khi cua: `|pitch|` p95 ~8°; cua trái ngả nhiều hơn cua phải (phần cơ khí, chưa xử lý).

## 3. Giữ vị trí, giữ hướng, chống đẩy

- Thả tay: hãm theo đường giảm tốc rồi chốt mốc vị trí khi xe yên (cửa sổ 200 ms sau khi hết lăn).
- Bị đẩy khi đang đứng: REC1 giữ **mốc vị trí cũ** (RT28) để xe quay về chỗ cũ, không trôi.
- REC1: giới hạn 50° / 1,2 m/s / 1 m / 4 s; phanh do nhiễu **không** bị cắt ở 4 s (RT29).
- Va chạm rất mạnh (>900 °/s) vẫn cắt motor — an toàn được ưu tiên hơn “không bao giờ ngã”.

## 4. Các đường tự cắt motor

13 mã fault, mô tả đầy đủ ở [bảng mã lỗi](Bang_Ma_Loi_Tu_Dong_Dung.md). Đáng chú ý khi đọc log:

- `fault 4` = nghiêng quá giới hạn (25° thường, 50° khi REC1) — cắt motor.
- `fault 16` = vượt trần tốc độ (200 ms liên tục, hoặc >1,45 m/s tức thời).
- `fault 4096` = REC1 hết 4 s **hoặc** phanh quỹ đạo không xong trong 4 s (phanh do nhiễu được miễn).
- `state=5, fault=1` ở cuối file là **STOP bình thường**, không phải lỗi.

## 5. Gateway và log

- SoftAP `TWIP_RACER_S3` / `12345678`, HTTP :80, WebSocket :81, trang `/ui/r12`.
- Log 122 byte/khung, 40 cột, 50 Hz; RAM ~6 phút; tải log sau khi STOP.
- **Không ghi Flash khi xe đang chạy** (RT33/NET34) — tránh hỏng vùng log khi nguồn sụt.
- TX WiFi 11 dBm để giảm đỉnh dòng; đã hết `reset=brownout` trong log mới nhất.
- Vùng log Flash hiện **chưa mount được** (`archive_error=1`) → cần format một lần, xem
  [RT33](RT33_On_Dinh_Ket_Noi.md) mục 3.

## 6. Mô phỏng

Mô hình dùng **chính header C của firmware**, đường đặc tính bánh được fit lại từ log xe thật
(lệch 1–2 %) — xem [RT35](RT35_Mo_Phong_Khop_Log.md). 36 ca kiểm tra: 12 tốc độ + 12 biên + 12 khởi
hành 50 cm/s, tất cả 20 s.
