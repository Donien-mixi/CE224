# RT38 — Chuyển hướng liên tục, hết khựng khi đổi lệnh và ở 90 cm/s

Người vận hành báo trên RT37: ở **90 cm/s** xe cân bằng không ổn định, dễ trôi và điều khiển bị
khựng; ở **50 và 80 cm/s**, đổi hướng đột ngột thì xe phải khựng lại để giữ cân bằng thay vì chạy
liên tục theo lệnh. Yêu cầu: cả ba mốc đều mượt, ổn định, thực thi đúng tín hiệu thay đổi liên tục.

Chưa có log RT37 cho đúng các lượt này. Bằng chứng định lượng dưới đây lấy từ log **RT36 `(2)(5)`**
(7 699 mẫu / 154 s) — cùng luồng điều khiển, trước RT37 hai hằng số ngưỡng nghiêng.

## 1. Số đo tại các cú đổi hướng (log RT36 `(2)(5)`)

| Sự kiện | `v` lúc đổi | Phanh trong 1,2 s | REC1 (phase 2) | Về lại 0,5 m/s |
|---|---:|---:|---:|---:|
| +0,81 → −1 (đảo chiều) | +0,81 m/s | **72 %** | 0 % | không đạt |
| −0,91 → +1 (đảo chiều) | −0,91 m/s | 52 % | **52 %** | không đạt |

Cùng lúc, toàn log có **599 mẫu phase 2** rải rác — mỗi lần là một cú vào REC1 ngắn do gyro của
chính cú đảo chiều, kéo theo hạ trần tốc độ 0,25/0,50 m/s trong 1,5 s.

## 2. Ba nguyên nhân trong code

| # | Cơ chế | Hệ quả |
|---|---|---|
| 1 | Nút điều khiển gửi `neutral` vài gói khi đổi hướng; `BalanceMotion` phanh ngay gói neutral đầu | xe dừng–đi lại mỗi lần đổi hướng |
| 2 | Khi mốc vị trí vượt xe **20 cm**, `target` bị đặt **0** thay vì giữ tốc độ lệnh | ở 0,90 m/s mốc chạy trước nhanh → tốc độ tụt rồi ramp lại: khựng và trôi |
| 3 | REC1 kích hoạt **tức thì** khi gyro ≥ 150 °/s kể cả khi đang giữ lệnh; cú đảo chiều thật sự tạo spike 150–250 °/s | vào REC1 sau mỗi lần đảo chiều, trần tốc độ 0,25/0,50 m/s |

## 3. Sửa gì (chỉ lớp giám sát, không đổi R12)

| # | Tệp | Thay đổi |
|---|---|---|
| 1 | [`balance_motion.h`](../Xe_Can_Bang/Core/Inc/balance_motion.h) | Giữ lệnh cũ tối đa **120 ms** khi nhận neutral; lệnh mới trong cửa sổ đó là **đảo chiều liên tục**, không phanh. Nhả hẳn >120 ms vẫn phanh; mất gói (stale) vẫn phanh ngay |
| 2 | `balance_motion.h` | Mốc vượt 20 cm: **neo mốc tại vị trí xe +20 cm và giữ tốc độ lệnh** khi xe đang chạy (≥0,30 m/s); nếu xe đứng (kẹt) vẫn cắt lệnh như cũ để bộ phát hiện tường huỷ |
| 3 | [`balance_recovery.h`](../Xe_Can_Bang/Core/Inc/balance_recovery.h) | Khi **đang giữ lệnh**, spike gyro phải duy trì **60 ms** mới vào REC1 (giống đường nghiêng); khi **không có lệnh** vẫn vào ngay. Ngưỡng 150 °/s giữ nguyên |

Không đổi gain/observer/phân bổ lực R12, trần cua, neo mốc khi đứng yên, phanh 0,85, trim 110 PWM,
khung 122 B, lease 350 ms, cắt góc/quãng/I2C.

## 4. Kiểm chứng

- **39/39 host test pass**. Hai test mới, đều có old-constant probe phải fail:
  - `test_rt38_continuous`: +0,90 → 3 gói neutral → −0,90 **không phanh**, tới −0,90; nhả hẳn 40 gói
    thì phanh và về 0; mốc vượt 20 cm (xe chỉ chạy nửa tốc độ) vẫn giữ `v_ref` 0,90 và lead ≤0,205 m.
  - `test_rt38_recovery`: không lệnh + gyro 200 → REC1 ngay; có lệnh + 1 mẫu gyro 200 → không;
    đủ 12 mẫu (60 ms) → vào; gyro 90 không vào.
- Các test cũ cập nhật theo chính sách mới (có ghi lý do trong test): `test_rt19_hold` (nhả hẳn phải
  chờ 24 gói), `test_rt18_launch`/`test_balance_recovery`/`test_rt12_replay72` (gyro khi có lệnh cần
  60 ms), `test_rt8_ab` (mốc vượt 20 cm giữ tốc độ thay vì cắt).
- Mô phỏng sau sửa: **12/12** @MAX 80, **12/12** biên, **12/12** khởi hành 50, **11/12** @MAX 90 —
  ca còn lại vẫn là pin 11,2 V như RT35/RT36/RT37, không phát sinh ca mới. Bộ `--disturbance`
  (xô 1–4 N) vẫn khó như thiết kế, không dùng để kết luận “không ngã”.
- Build: STM32 0 lỗi 0 cảnh báo; ESP32 1 090 907 B. Hash trong
  [`Firmware/RT38_CONT_20261118/`](../Firmware/RT38_CONT_20261118/README.md).

## 5. Chưa kiểm trên xe

RT38 mới qua host test + mô phỏng. Cần một lượt log đúng các tình huống đã báo:

1. Đứng yên 30 s ở 90.
2. 90 tiến 8 s, lùi 8 s.
3. 50 và 80: tiến → đổi sang phải/trái liên tục 5 lần, không nhả ga lâu.
4. Đang tiến 0,8 → đảo lùi ngay; và ngược lại.

Đạt khi: không có `fault`, `phase 2` gần như không xuất hiện sau đảo chiều, `v_ref` không tụt về 0
giữa lượt, tốc độ giữ đúng mốc, `x` không trôi quá 40 cm.

## 6. Nạp

`Firmware/RT38_CONT_20261118/nap_rt38.bat` — **nạp cả hai board**. Banner `RT38-CONT-NET38`.
