# RT34 — Cua mượt ở MAX 90: trần tốc độ do đường

Ngày 17/11/2026. Bản chốt trên `main`, **nạp cả hai board**.

> **RT36 (18/11) kế tiếp:** trần cua hạ 0,95 → 0,90 (cua ~0,749 m/s), trần ngang theo tốc độ lệnh,
> neo mốc chỉ khi duty > 0,95, nhả mốc khi đứng yên, phanh nhả 0,85 m/s². Xem
> [`RT36_Sua_4_Loi.md`](RT36_Sua_4_Loi.md). Bản RT34 vẫn là bản **đã có log xe 173 s**.

## 1. Vì sao cua bị giật (số liệu log 38, RT32 ở MAX 90)

| Hiện tượng | Đo được |
|---|---|
| Yaw vọt trên tham chiếu khi vào cua | **+37 %** (1,15 → 1,58 rad/s), hồi dần ~2 s |
| Bánh ngoài bão hoà | PWM **2499 (100 %)** |
| Xe tự tụt tốc trong cua | **0,90 → 0,66 m/s** dù `v_ref` vẫn 0,90 |
| Governor ghìm (cờ 16384) | 50–100 % thời gian trong cua |

Nguyên nhân: bánh ngoài phải chạy `v + |w|·0,131`. Ở 0,90 m/s với cua 1,15 rad/s cần **1,05 m/s**,
động cơ chỉ tới ~0,95 m/s (pack 11,7 V) ⇒ bão hoà ⇒ vòng yaw R12 (đóng băng) vọt rồi kéo lại.
Cua ở MAX 80 không bị vì 0,80 + 0,15 = 0,95 vừa đúng trần.

## 2. Cách sửa

`balance_turn.h` đặt `BalanceMotion_corner_budget()` mỗi tick; `balance_motion.h` kẹp **tham chiếu
tốc độ** (chỉ hạ) vào giới hạn:

```
giới hạn = 0,95 − |w_ref| · 0,131      (chỉ khi |w_ref| > 0,30)
```

- Đi qua ramp `BALANCE_MOTION_DECEL` (0,90 m/s²) ⇒ mượt; `x_ref` tích phân theo `v_ref` đã kẹp nên
  không sinh sai lệch vị trí.
- MAX 90 + cua 1,15 → tham chiếu **0,80 m/s** (bánh ngoài 0,95 = đúng trần).
- MAX 80 + cua, và chạy thẳng 0,90 → **không đổi**.
- Không đổi gain/observer/phân bổ lực R12, khung 122 B, lease/STOP/REC1/interlock/bù bánh trái.

Tham số có `#ifndef`: `BALANCE_TURN_VNL_BUDGET` (0,95), `BALANCE_TURN_HALF_TRACK` (0,131).

## 3. Kiểm chứng

- **32/32 host test**, gồm `test_rt34_corner`: thẳng 0,90 giữ 0,90; vào cua hạ về **0,803** và
  **không tick nào nhảy quá 0,012**; nhả cua về 0,90; preset 80 không đổi; **probe tắt giới hạn phải fail**.
- Mô phỏng: stress@80 12/12, edge 12/12, launch50 12/12; stress@90 **11/12** sau khi fit lại plant theo
  log xe ([RT35](RT35_Mo_Phong_Khop_Log.md)) — ca còn lại là 11,2 V, vùng bị cấm với mức 90.
- Sửa thêm lỗi test cũ: `test_rt24_sym.cjs` ghi 281 mẫu vào mảng `samples[117]` (tràn mảng) — nay
  `samples[600]` có kiểm biên; test vẫn khẳng định đúng hành vi trước đây.
- **Đã kiểm trên xe**: log RT34 173 s ở pack 11,7 V — 0 ngã, 0 tự ngắt, cờ 512 trong cua còn 19–26 %
  (trước 50–77 %).

## 4. Điều kiện vận hành

- MAX 90 chỉ khi pin **≥ 12,0 V**; 11,7–12,0 V dùng 50/80.
- Cua ở MAX 90 **tự giảm về ~0,80 m/s** — đó là hành vi mong muốn.
- Không bấm “SỬA VÙNG LOG FLASH” khi xe đang cân bằng (xem RT33).

## 5. Giới hạn

Vừa 0,90 vừa cua gắt là bất khả thi trên 3S (bánh ngoài cần 1,05 m/s > trần động cơ). Muốn vậy
phải lên 4S. Bản này đổi “cua gắt nhưng giật” thành “cua êm, chậm lại đúng mức vật lý cho phép”.
