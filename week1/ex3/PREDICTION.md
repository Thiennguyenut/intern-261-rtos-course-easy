# Dự đoán — Week 1 / Ex3

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2413284   `PERIOD_MS` = 7 ms   `WORK_JITTER_US` = 2400 us    `SAMPLE_COUNT` = 800

1. Bảng dự đoán (điền trước khi đo):

| # | Delay | Tick | Chu kỳ tb (ms) | Jitter (us) | Drift tổng sau SAMPLE_COUNT (ms) |
|---|---|---|---|---|---|
| 1 | `vTaskDelay` | 1000 | ~7.75 | ~2000 | ~600 ms |
| 2 | `xTaskDelayUntil` | 1000 | ~7 | ~0 | ~0 |
| 3 | `vTaskDelay` | 100 | | | |
| 4 | `xTaskDelayUntil` | 100 | | | |

- Giả định tải giả mang phân phối đều, tức là mọi giá trị tải có xác suất bằng nhau.
- Phần tick 100 Hz không khảo sát theo xác nhận của anh Triều.

2. Cấu hình nào có drift lớn nhất? Vì sao, viết bằng công thức có `WORK_JITTER_US`.
**Trả lời:**
- Dự kiến vTaskDelay có drift lớn hơn vì nó lấy tick tại lúc gọi làm mốc.
- Công thức: Drift = SAMPLE_COUNT × (1000 + 2(WORK_JITTER_US - 2000 + 1))/(WORK_JITTER_US + 1)
- vTaskDelay dự kiến có drift lớn hơn vì tải có thể làm lời gọi vượt một hoặc hai tick. Hàm tick tại lúc gọi cộng thêm 7 tick, nên những tick đã trôi qua làm lịch bị đẩy về sau và sai lệch tích lũy.
- xTaskDelayUntil giữ mốc lịch trước cộng 7 tick. Vì tải tối đa 2,4 ms < 7 ms, task dự kiến hoàn thành kịp và bám lịch, nên drift gần 0.
3. `PERIOD_MS` = 7 ms  . Với tick 100 Hz, một tick = 10 ms, nên chu kỳ thực tế sẽ
   bị làm tròn chuyển thành xuống 0 ms. Sai số hệ thống: 100 %.
- Với xác nhận của anh Triều, phần ex3 em chỉ làm 2 cấu hình là vTaskDelay và xTaskDelayUntil với cấu hình là 1000. Do đó em không thể đo thực tế với tick 100 Hz.
4. Jitter ở tick 1000 Hz so với 100 Hz: lớn hơn / nhỏ hơn / bằng? Vì sao?
**Trả lời:**
- Vì với tick rate 100 Hz thì 2 hàm `vTaskDelay` và `xTaskDelayUntil` sẽ bị assert nên không thể so sánh. Do đó không khảo sát theo xác nhận của anh Triều.
5. Nếu task này **không** phải ưu tiên cao nhất, đại lượng nào trong bảng bị ảnh hưởng
   nhiều nhất?
**Trả lời:**
- Đại lượng jitter bị ảnh hưởng nhiều nhất.
- Vì task có thể đã Ready nhưng chưa được CPU chạy vì task ưu tiên cao hơn đang chạy. Thời điểm toggle GPIO vì thế có thể sẽ bị thực hiện muộn. Do đó Jitter sẽ bị ảnh hưởng nhiều nhất.

