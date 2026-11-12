# Dự đoán — Week 3 / Ex4

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2413284

1. Chạy `app/deadlock_demo.c` y nguyên như được cấp. Dự đoán: hệ thống sẽ
   treo sau khoảng bao lâu (ngay lập tức, hay sau vài chu kỳ)? Vì sao không
   phải ngay lập tức (gợi ý: cần cả 2 task "đua" vào đúng thời điểm)?
   **Trả lời:**
   - Dự đoán hệ thống sẽ treo sau vài chu kỳ.
   - Không phải ngay lập tức vì Deadlock chỉ xảy ra khi mỗi task đã giữ một mutex và đang chờ mutex còn lại. Lúc khởi động, không task nào giữ khóa nào. `task_alpha` lấy s_mutex_a rồi ngủ 5 ms, và trong lúc đó `task_beta` được chạy, lấy s_mutex_b rồi cũng ngủ 5 ms. Chỉ khi hai task đã ngủ xong trong trạng thái giữ khóa như vậy thì chúng mới cùng xin khóa còn lại và bị block. Vì vậy deadlock cần cả hai task “đua” vào đúng thứ tự.

2. Khi treo, `uxTaskGetSystemState` sẽ báo trạng thái gì cho `task_alpha` và
   `task_beta`? Dự đoán tên mutex mỗi task đang chờ (dựa trên đọc code, chưa
   chạy công cụ).
   **Trả lời:**
   - `task_alpha`: eBlocked, nó đang chờ s_mutex_b.
   - `task_beta`: eBlocked, nó đang chờ s_mutex_a.
   - `Idle task`: eReady.

3. Tính reload value cho IWDG với `IWDG_TIMEOUT_MS` của bạn (LSI ~40 kHz).
   **Trả lời:**
   - Với `IWDG_TIMEOUT_MS` = 250 và `LSI_HZ` ≈ 40000 và chọn prescaler /4 thì:
   reload = IWDG_TIMEOUT_MS × (LSI_HZ / prescaler) / 1000 = 250 × (40000 / 4) / 1000 = 2500
   - Với reload 12 bit thì phạm vi từ (0-4095), do 2500 ≤ 4095 nên hợp lệ.

4. Sau khi sửa lỗi deadlock và thêm supervisor + IWDG, nếu bạn cố tình tái
   tạo lại tình huống deadlock cũ (tạm thời), dự đoán board sẽ phản ứng thế
   nào và sau bao lâu.
   **Trả lời:**
   - Khi cố ý đưa lại deadlock cũ, diễn biến sẽ như sau:

      - Hai task bị kẹt trước khi gọi supervisor_checkin, vì check-in được đặt sau khi nhả cả hai mutex. 
      - Do đó Supervisor không nhận đủ bit, nên xEventGroupWaitBits trả về không đủ ALL_BITS và supervisor không refresh IWDG.
      - Lúc này IWDG đếm xuống 0 và reset board. Vậy Board sẽ reset khoảng 250 ms sau lần refresh cuối cùng trước khi treo. 

