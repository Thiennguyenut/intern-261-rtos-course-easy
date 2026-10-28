# Dự đoán — Week 2 / Ex3

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2413284   `LOW_HOLD_MS` = 8 `MID_WORK_MS` = 20
`HIGH_PERIOD_MS` = 50   `DEADLINE_MS` = 25

1. Cấu hình binary semaphore: thời gian `task_high` bị block dự đoán ≈ 20 ms.
   Phép tính dựa trên đâu (liệt kê các thành phần cộng lại)?
   - Giả sử ban đầu task_mid và task_high đều đang Blocked nên task_low được chạy lần đầu, take semaphore và lưu mốc DWT bắt đầu. Ngay sau đó task_high trở thành Ready, preempt task_low vì có priority cao hơn. task_high không take được semaphore do task_low đang giữ nên task_high chuyển sang Blocked. task_mid trở thành Ready ngay sau đó, có priority cao hơn task_low nên được chạy trước task_low. task_mid hoàn thành công việc và delay thì task_low mới được chạy tiếp. task_low kiểm tra DWT thấy đã vượt 8 ms, thoát vòng chờ rồi give semaphore, sau đó task_high được đánh thức và lấy được tài nguyên.
   - Task_high BLOCKED ≈ task_mid + overhead

2. `task_high` có đạt `DEADLINE_MS` ở cấu hình binary semaphore không? Vì sao?
   - `task_high` **đạt deadline** ở cấu hình binary semaphore. Vì Response time ≈ 20 ms chờ + 1 ms xử lý + overhead. Dự đoán overhead nhỏ hơn 4 ms nên Response time < `DEADLINE_MS` = 25 ms

3. Cấu hình mutex: thời gian `task_high` bị block dự đoán = 8 ms. Vì sao
   ngắn hơn (hoặc không) so với câu 1?
   - Em dự đoán `task_high` bị block **khoảng 8 ms**, cộng ảnh hưởng của overhead. 
   - vì task_high yêu cầu ngay sau khi task_low bắt đầu chờ bận. Khi task_high block trên mutex, task_low kế thừa priority của task_high nên task_mid không preempt được task_low. task_low chạy tiếp, đến khoảng 8 ms thì give mutex và task_high lấy được tài nguyên. Khoảng chờ giảm từ khoảng 20 ms xuống khoảng 8 ms nhờ priority inheritance.

4. Trên ảnh logic analyzer ở cấu hình binary semaphore, kênh `task_mid` sẽ ở
   trạng thái nào trong lúc `task_high` bị chặn — chạy liên tục, chạy ngắt
   quãng, hay không chạy? Vì sao?
   - Ở cấu hình binary semaphore, kênh task_mid dự kiến chạy liên tục trong khoảng 20 ms. Vì task_high đang Blocked do task_low còn giữ semaphore nên task_high không được scheduler chọn. task_low ở trạng thái Ready, nhưng task_mid cũng Ready và có priority cao hơn task_low nên task_mid preempt task_low. task_mid không cần semaphore nên vẫn tiếp tục công việc cho đến khi đủ 20 ms rồi gọi vTaskDelay(). Khi đó task_low mới được chạy tiếp để give semaphore, giúp task_high được đánh thức. Binary semaphore không có priority inheritance nên task_low không được nâng priority để chạy trước task_mid.

5. Nếu đổi `task_mid` sang ưu tiên **thấp hơn** `task_low`, inversion còn xảy ra
   không? Vì sao (đây là cách phân biệt priority inversion với việc chỉ đơn
   giản là "task_low chạy lâu").
   - Nếu đổi `task_mid` sang ưu tiên **thấp hơn** `task_low` thì không xảy ra priority inversion. Vì Không còn phần kéo dài thời gian chờ do task_mid chen vào theo cơ chế ba task đang xét. Với thứ tự task_high > task_low > task_mid, sau khi task_high block, task_low được chạy tiếp trước task_mid và trả semaphore vào khoảng 8 ms.

