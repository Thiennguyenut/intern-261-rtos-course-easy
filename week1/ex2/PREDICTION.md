# Dự đoán — Week 1 / Ex2

> **Commit TRƯỚC khi nạp chương trình.**

`student_id`: 2413284    Ưu tiên của tôi: A= 1 B= 3 C= 4

1. Bước 2 (`configUSE_PREEMPTION`=1, ưu tiên khác nhau): task nào chạy? task nào
   **không bao giờ** chạy? CPU share dự đoán:
- task chạy: task C.
- task không bao giờ chạy: task A và B.
- Task C được chọn vì có priority cao nhất (4). C busy-loop liên tục, không block nên luôn sẵn sàng chạy. Task A và B vẫn ở Ready nhưng có priority thấp hơn task C, vì vậy scheduler tiếp tục chọn task C ngay cả khi tick xảy ra. A và B bị starvation. Không có preemption giữa các task vì không có task Ready nào ưu tiên cao hơn C hoặc cùng priority với C để time slicing. 

   | Task | CPU share dự đoán (%) |
   |---|---|
   | A | 0% |
   | B | 0% |
   | C | 100% |

2. Bước 3 (`configUSE_PREEMPTION`=0): dự đoán thay đổi gì so với bước 2? Vì sao?
**Trả lời:**
- Nếu Cả ba task được tạo thành công trước khi scheduler khởi động. Scheduler ban đầu vẫn chọn C. Vì C không block hoặc yield nên vẫn sẽ tiếp tục chạy. Tắt preemption không tự tạo cơ hội chạy cho A và B.
- vì tắt preemption, scheduler không chuyển sang task có priority cao hơn chỉ vì task đó Ready. Nên khi task C chạy busy-loop liên tục, không block thì task A, B vẫn bị starvation.
3. Bước 4 (3 task cùng ưu tiên): time slice dự đoán = 1 ms. Dựa vào đâu?
**Trả lời:**
- Với configUSE_PREEMPTION = 1 và configUSE_TIME_SLICING = 1, ba task có priority như nhau, đều ở trạng thái Ready sẽ được luân phiên theo tick. Với cấu hình configTICK_RATE_HZ = 1000, time slice sẽ là 1 ms. Scheduler có thể lập lịch luân phiên theo mỗi tick có thể minh họa như: A → B → C → A → B → C → A → B → C →...
4. Bước 5: tôi sẽ sửa starvation bằng cách: Tạo khoảng Blocked cho mỗi task . Cách này đánh đổi gì?
**Trả lời:**
- Ta có thể chuyển trạng thái của các task đang chạy bằng cách sử dụng vTaskDelay(tick).
- Đánh đổi: Giảm số vòng thực hiện mỗi giây và thêm khoảng chờ phụ thuộc tick. Tuy nhiên không thể dự đoán CPU share bằng nhau vì các task vẫn khác priority và thời gian làm việc.
5. Chân nào sẽ không bao giờ đổi trạng thái trên logic analyzer ở bước 2? Chân PA0 (task A) và PA4 (task B).
