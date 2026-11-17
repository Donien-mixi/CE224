# RT32 — Bộ tốc độ chốt 50 / 80 / 90 cm/s và bù lệch bánh trái

Ngày 17/11/2026. Firmware **chính thức trên `main`**, thay `RT29-NOCUT`. Quyết định của người vận
hành sau hai vòng thử: RT30 (0,86) và RT31 (0,90).

## 1. Bộ tốc độ chốt

| Preset | Trước (RT29) | **Nay (RT32)** | Duty cần (log thật) |
|---|---:|---:|---|
| 0 – chậm | 0,50 m/s | **0,50 m/s** | 0,53 @11,7 V |
| 1 – vừa | 0,72 m/s | **0,80 m/s** | 0,790 @12,2 V · 0,823 @11,7 V · 0,859 @11,2 V |
| 2 – MAX | 0,80 m/s | **0,90 m/s** | 0,879 @12,2 V · 0,900 @11,9 V · 0,955 @11,2 V |

- `BALANCE_STOP_SPEED` (trần thường dùng cho interlock/startup) nâng 0,80 → **0,90**.
- `BALANCE_TURN_WHEEL_ENV_72` **giữ 0,98** dù preset 1 lên 0,80: một cửa chéo 1,15 rad/s ở 0,80 chỉ
  cần bánh ngoài 0,95 m/s, vẫn dưới 0,98; nâng nữa chỉ làm lệnh vượt VNL (0,92–0,96 m/s ở pin yếu)
  mà không sinh thêm mô-men.
- UI: ba nút **50 / 80 / MAX 90**; build ID hai board RT32/NET33.

**Điều kiện vận hành bắt buộc** (firmware không đo pin): MAX 90 chỉ dùng khi pin đo **≥ 12,0 V**;
11,7–12,0 V dùng 50/80; dưới 11,7 V chỉ 50/80. Bảng đầy đủ trong
`Firmware/RT34_CORNER_20261117/README.md` (bản hiện hành, đã gồm 50/80/90).

## 2. Bù lệch bánh trái

Đo trên log thật (RT30 0,86 và RT31 0,90): để cùng tốc độ, **bánh trái cần thêm ~98 PWM khi tiến,
~160 khi lùi, ~166 khi quay tại chỗ** (≈110 PWM = 4,4 % thang) — bất đối xứng cơ khí
(motor/hộp giảm tốc/ma sát), không phải lỗi encoder hay dấu.

Cách làm (`balance_trim.h` + một chỗ gọi trong `robot_fsm.c`):

```
pwm_left = BalanceTrim_Left(output[0], BALANCE_TRIM_LEFT_PWM=110, MOTOR_MAX_PWM=2499)
```

- Cộng/trừ **theo dấu quay** của bánh trái, chết vùng ±0,5 PWM, kẹp ±2499, giữ NaN.
- Đặt **sau** bộ phân bổ R12: không đổi gain, observer, `cat_allocate`, khung 122 B hay bất kỳ giới
  hạn an toàn nào. Bánh phải không đổi.
- **Giới hạn trung thực:** bù này làm hai bánh cùng tốc độ ở cùng duty (đúng mô-men) và giảm offset
  mà vòng giữ hướng phải giữ; nó **không** làm bánh trái khoẻ thêm, nên tốc độ quay tối đa khi bánh
  trái là bánh ngoài (rẽ phải khi tiến) vẫn bị chặn bởi vật lý. Nếu muốn cân bằng hẳn thì phải đổi
  motor/hộp giảm tốc bên trái hoặc thêm luật riêng cho chiều quay phải — cần một vòng test nữa.

## 3. Kiểm chứng

| Hạng mục | Kết quả |
|---|---|
| Host test | **31/31 pass**; `test_rt32_set` kiểm 3 mốc 0,50/0,80/0,90, trần 0,90, bù 110 PWM (dấu, kẹp, NaN) và **probe 0,72/0,80/không bù phải fail** |
| Mô phỏng `--stress` ở MAX 80 | **12/12** |
| Mô phỏng `--stress` ở MAX 90 | **9/12** — 3 ca ngã: hai ca **11,2 V** và ca **motor yếu 20 %** (đúng vùng bị cấm ở mục 1) |
| `--stress --edge` | 12/12 |
| `--stress --launch --launch50` | 12/12 |
| Hash nguồn ↔ binary | 13/13 khớp (`SOURCE_SHA256.json`) |

Tổng: 33/36 ca khi MAX 90, **36/36 khi chọn đúng mức theo điện áp**. Ba ca ngã là giới hạn vật lý đã
biết, không phải hồi quy — nhưng vì vậy **không được chạy MAX 90 với pin cạn**.

## 4. So với bản chốt trước (RT29, 0,80)

| Chỉ số (log xe thật) | RT29 0,80 | RT32 0,90 (bản thử) |
|---|---|---|
| Vận tốc p95 / max | 0,837 / 0,862 | 0,906 / 0,938 |
| duty p50 / p95 | 0,718 / 0,854 | 0,765 / 0,921 |
| Governor ghìm | 5 % | 17 % |
| Pitch p95 | 9,39° | 8,52° |
| `\|x−x_ref\|` max | 0,176 m | 0,265 m |
| Tự ngắt 4096 | 3 lần (đã sửa ở RT29) | 0 |

Nhanh hơn ~8 % ở p95, nghiêng tốt hơn, nhưng sát ngưỡng duty và sai số vị trí lớn hơn — đó là lý do
có bảng điện áp ở mục 1.

## 5. Việc tiếp theo (nếu muốn hoàn thiện hơn)

1. Log xe thật cho RT32 (đủ 3 mức, ghi điện áp) để xác nhận bù bánh trái có tác dụng trong thực tế.
2. Nếu vẫn thấy rẽ phải khi tiến yếu: đo lại độ lệch sau bù; nếu còn, phải xử lý cơ khí hoặc thêm
   luật yaw riêng cho chiều phải (một vòng test riêng, không gộp vào bản này).
3. Muốn >1,0 m/s: cần pack 4S (xem `Docs/RT31_Toc_Do_090.md`, mục 4S).
