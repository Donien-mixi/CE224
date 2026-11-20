# Bản chốt RT44 — cua đều 4 hướng (trần yaw theo duty còn dư)

| Board | Build ID |
|---|---|
| STM32F411 | `BAL-20261120-R12-8DIR-RT44-TURN` |
| ESP32-S3 | `TWIP-WEB-20261120-BAL-R12-8DIR-RT44-TURN-NET44` |

## 1. Đo được gì (log RT42 148,6 s + RT41 50,4 s, MAX 90, cùng lệnh 1,15 rad/s)

| dir | turn | n mẫu | `w_ref` | `w_meas` | tỉ lệ | dao động | % mẫu vượt 10 % | pitch | duty trái | duty phải | ghim trái | v trái | v phải |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| −1 | −1 | 368 | 1,15 | **0,90** | 0,78 | 0,38 | 17 | 2,9° | 0,60 | 0,71 | 0 % | −0,56 | −0,67 |
| −1 | +1 | 450 | 1,15 | **1,38** | **1,20** | 0,36 | **64** | 2,0° | 0,87 | 0,48 | **14 %** | −0,82 | −0,47 |
| +1 | −1 | 730 | 1,15 | **1,31** | **1,14** | 0,24 | **59** | 4,0° | 0,86 | 0,53 | 3 % | +0,82 | +0,50 |
| +1 | +1 | 962 | 1,15 | 1,16 | 1,01 | 0,05 | 7 | 4,5° | 0,55 | 0,84 | 0 % | +0,52 | +0,82 |

Cùng một lệnh 1,15 rad/s nhưng yaw thực tế trải từ **0,90 đến 1,38 rad/s (lệch 53 %)**, có hướng vượt
**20 %** và dao động tới **0,38 rad/s** — đúng hiện tượng “cua không mượt, xe bị văng”.

Nguyên nhân: hai hướng xấu nhất là hai hướng cần **bánh trái chạy nhanh nhất** (lùi-trái, tiến-phải).
Bánh trái vốn cần ~11 % duty nhiều hơn bánh phải (cơ khí), nên nó chạm trần duty **trước** khi bộ giới
hạn cũ (ngưỡng 96 %) kịp phản ứng → vi sai vọt lên rồi dao động.

## 2. Sửa

| Tệp | Thay đổi |
|---|---|
| [`balance_turn.h`](../../Xe_Can_Bang/Core/Inc/balance_turn.h) | Hệ số dư duty giờ nhân vào **cả tham chiếu yaw** (trước chỉ nhân vào budget tuyến tính của cua); **ngưỡng hạ 96 → 92 %**, hồi khi **≤ 86 %** |

Tác dụng: khi một bánh sắp chạm trần, cua **quay chậm lại một chút** thay vì vi sai vọt rồi giật. Bốn
hướng bị cùng một luật nên đều nhau hơn; cua ở mức bình thường (duty < 92 %) **không bị ảnh hưởng**.

## 3. Kiểm chứng

- **42/42 host test**; các probe cũ của `test_rt40_hold` vẫn fail đúng.
- Build STM32 84 284 B, ESP32 1 094 304 B; hex build trong IDE (`D:\CubeIDE_ws_verify`) **trùng SHA256**
  với bản đóng gói (`0ee37a5c…`). 7 binary + 16 nguồn trong `SHA256.json`/`SOURCE_SHA256.json`.

## 4. Nạp và kiểm

`nap_rt44.bat` (nhập COM ESP32) hoặc menu → **1**. Banner phải là **`RT44-TURN-NET44`**.

Kỳ vọng ở log sau (MAX 90, rẽ đủ 4 hướng, mỗi hướng ≥ 2 s):

| Chỉ số | Trước | Kỳ vọng |
|---|---:|---:|
| Trải yaw giữa 4 hướng | 0,90–1,38 rad/s | **1,05–1,20** |
| % mẫu vượt 10 % tham chiếu | 7–64 % | **< 15 %** |
| Dao động (median \|w_meas−w_ref\|) | 0,05–0,38 | **< 0,12** |
| Ghim duty một bánh khi cua | tới 14 % | **0 %** |
| Lệch bánh trái/phải khi chạy thẳng | 0,20 m/s | **< 0,15 m/s** |

## 5. Giới hạn

- **Chưa kiểm trên xe.** Chỉ là hệ số/ngưỡng của lớp giám sát, không đổi R12 hay quỹ đạo.
- Nếu log mới vẫn lệch > 0,15 m/s giữa hai bánh khi **chạy thẳng**, phần còn lại là **cơ khí bánh trái**
  (bạc đạn/belt/motor) — không sửa được bằng gain; khi đó nên kiểm cơ khí thay vì chỉnh tiếp.
