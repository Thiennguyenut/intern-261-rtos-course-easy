# Dự đoán — Week 2 / Ex4

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2413284    `EVENT_BIT_COUNT` = 4  `SYNC_TIMEOUT_MS` = 120 ms

1. RAM event group chiếm (bytes) dự đoán: 24 byte . Dựa vào đâu (cấu trúc dữ
   liệu nào trong FreeRTOS)?
   - Em dự đoán 24 bytes, dựa vào cấu trúc EventGroup_t trong event_groups.c bao gồm: 4 bytes lưu event bits và 20 byte lưu danh sách task đang chờ.

2. RAM thêm khi dùng task notification cho 1 điểm đồng bộ: dự đoán = 0 bytes. Vì sao?

   - Em dự đoán 0 byte cấp phát thêm khi bắt đầu dùng notification của `sync_task`.
   - Vì `notification value` và `notification state` đã nằm sẵn trong TCB của `sync_task`. xTaskNotifyGive sử dụng trực tiếp các trường đó nên không tạo thêm object khác.

3. Dự đoán thời gian đánh thức (producer gọi → sync_task chạy) của notification
   so với event-group-bit: nhanh hơn / chậm hơn / xấp xỉ? Vì sao (nghĩ đến số
   lớp hàm bên dưới mỗi API)?

   - Em dự đoán thời gian đánh thức trung bình của notification nhỏ hơn event-group-bit: notification_tb < event_group_tb.
   - Vì Notification truy cập trực tiếp task nhận thông qua handle và cập nhật notification trong TCB, còn Event group phải cập nhật bit, duyệt danh sách các task chờ và kiểm tra điều kiện bit trước khi đưa task phù hợp về Ready.

4. Nếu `sync_task` bị timeout (`SYNC_TIMEOUT_MS` hết hạn) vì một producer
   không set bit kịp, `xEventGroupWaitBits` trả về gì? Làm sao phân biệt được
   "đủ bit" với "timeout"?
   - Với bốn nguồn, mask cần chờ là 0x0F (tức là 1111) nên nếu một producer không set bit kịp trong thời gian timeout thì `xEventGroupWaitBits` sẽ trả về giá trị hiện tại của các flag event bits. 
   - Để phân biệt được đủ bit với timeout thì phải thỏa điều kiện: (returned_bits & 0x0F) == 0x0F. Nếu điều kiện đúng thì đã đủ bit còn điều kiện sai thì chứng tỏ là chưa đủ bit trong thời gian timeout

5. Nêu trước một tình huống bạn nghĩ **không thể** thay toàn bộ event group
   bằng notification — sẽ kiểm chứng lại ở REPORT.md.

   - Tôi dự đoán không thể thay trực tiếp event group bằng xTaskNotifyGive / ulTaskNotifyTake trong trường hợp: sync_task cần chờ đủ 4 bit 1 của bốn nguồn độc lập, trong khi mỗi nguồn có thể báo nhiều lần trước các nguồn khác. Vì notification count chỉ lưu tổng số lần báo, không xác định nguồn nào đã báo. Một nguồn gửi bốn lần và bốn nguồn mỗi nguồn gửi một lần đều có thể tạo count bằng 4, nhưng chỉ trường hợp thứ hai đủ điều kiện đồng bộ.
