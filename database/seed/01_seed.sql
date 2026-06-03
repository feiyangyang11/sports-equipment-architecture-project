-- Sports equipment reservation and borrow/return management system
-- Initial development seed data
--
-- Notes:
-- 1. This file is designed for development and demo environments.
-- 2. It is idempotent for the seeded rows below.
-- 3. Password hashes in this file are generated with SHA2(plain_text, 256).
--    Plain text passwords for local development:
--      admin01    -> Admin@123
--      admin02    -> Admin@123
--      stu2026001 -> Student@123
--      stu2026002 -> Student@123
--      stu2026003 -> Student@123
--
-- Recommended execution order:
--   1) Run database/schema/01_init.sql
--   2) Run this file

USE `sports_equipment_management`;

START TRANSACTION;

-- ------------------------------------------------------------------
-- user
-- ------------------------------------------------------------------
INSERT INTO `user` (
  `id`, `username`, `password_hash`, `real_name`, `role`, `student_no`,
  `phone`, `email`, `status`, `last_login_at`, `created_at`, `updated_at`
) VALUES
  (1, 'admin01', SHA2('Admin@123', 256), '张老师', 'ADMIN', NULL,
   '13800000001', 'admin01@example.com', 'ACTIVE', '2026-06-01 09:00:00',
   '2026-05-20 08:00:00', '2026-06-01 09:00:00'),
  (2, 'admin02', SHA2('Admin@123', 256), '李老师', 'ADMIN', NULL,
   '13800000002', 'admin02@example.com', 'ACTIVE', '2026-06-01 09:10:00',
   '2026-05-20 08:10:00', '2026-06-01 09:10:00'),
  (101, 'stu2026001', SHA2('Student@123', 256), '王小明', 'STUDENT', '2026001',
   '13900000001', 'stu2026001@example.com', 'ACTIVE', '2026-06-01 10:05:00',
   '2026-05-20 08:20:00', '2026-06-01 10:05:00'),
  (102, 'stu2026002', SHA2('Student@123', 256), '李小红', 'STUDENT', '2026002',
   '13900000002', 'stu2026002@example.com', 'ACTIVE', '2026-06-01 10:10:00',
   '2026-05-20 08:30:00', '2026-06-01 10:10:00'),
  (103, 'stu2026003', SHA2('Student@123', 256), '赵子涵', 'STUDENT', '2026003',
   '13900000003', 'stu2026003@example.com', 'ACTIVE', '2026-06-01 10:20:00',
   '2026-05-20 08:40:00', '2026-06-01 10:20:00')
ON DUPLICATE KEY UPDATE
  `username` = VALUES(`username`),
  `password_hash` = VALUES(`password_hash`),
  `real_name` = VALUES(`real_name`),
  `role` = VALUES(`role`),
  `student_no` = VALUES(`student_no`),
  `phone` = VALUES(`phone`),
  `email` = VALUES(`email`),
  `status` = VALUES(`status`),
  `last_login_at` = VALUES(`last_login_at`),
  `updated_at` = VALUES(`updated_at`);

-- ------------------------------------------------------------------
-- equipment_category
-- ------------------------------------------------------------------
INSERT INTO `equipment_category` (
  `id`, `category_code`, `category_name`, `description`,
  `sort_order`, `status`, `created_at`, `updated_at`
) VALUES
  (1, 'BALL', '球类器材', '篮球、足球、排球等器材分类', 10, 'ACTIVE',
   '2026-05-20 09:00:00', '2026-05-20 09:00:00'),
  (2, 'RACKET', '拍类器材', '羽毛球拍、乒乓球拍等器材分类', 20, 'ACTIVE',
   '2026-05-20 09:05:00', '2026-05-20 09:05:00'),
  (3, 'FITNESS', '训练器材', '跳绳、哑铃等训练器材', 30, 'ACTIVE',
   '2026-05-20 09:10:00', '2026-05-20 09:10:00'),
  (4, 'OTHER', '其他器材', '其他体育教学和活动器材', 40, 'ACTIVE',
   '2026-05-20 09:15:00', '2026-05-20 09:15:00')
ON DUPLICATE KEY UPDATE
  `category_code` = VALUES(`category_code`),
  `category_name` = VALUES(`category_name`),
  `description` = VALUES(`description`),
  `sort_order` = VALUES(`sort_order`),
  `status` = VALUES(`status`),
  `updated_at` = VALUES(`updated_at`);

-- ------------------------------------------------------------------
-- equipment
-- ------------------------------------------------------------------
INSERT INTO `equipment` (
  `id`, `category_id`, `equipment_code`, `equipment_name`, `specification`,
  `storage_location`, `description`, `total_stock`, `available_stock`,
  `status`, `created_at`, `updated_at`
) VALUES
  (1001, 1, 'BALL001', '标准篮球', '7号 PU 室内外通用', '器材室 A-01',
   '用于篮球课程与社团活动', 20, 20, 'AVAILABLE',
   '2026-05-20 09:30:00', '2026-06-01 09:30:00'),
  (1002, 1, 'BALL002', '标准足球', '5号 训练比赛通用', '器材室 A-02',
   '用于足球训练与课外活动', 15, 14, 'AVAILABLE',
   '2026-05-20 09:31:00', '2026-06-01 09:31:00'),
  (1003, 1, 'BALL003', '标准排球', '室内软式排球', '器材室 A-03',
   '用于排球课程练习', 12, 12, 'AVAILABLE',
   '2026-05-20 09:32:00', '2026-06-01 09:32:00'),
  (1004, 2, 'RACKET001', '羽毛球拍', '碳素训练拍', '器材室 B-01',
   '单次借用通常为 1 至 2 支', 30, 30, 'AVAILABLE',
   '2026-05-20 09:33:00', '2026-06-01 09:33:00'),
  (1005, 2, 'RACKET002', '乒乓球拍', '横拍/直拍混合', '器材室 B-02',
   '乒乓球教学与活动使用', 18, 16, 'AVAILABLE',
   '2026-05-20 09:34:00', '2026-06-01 09:34:00'),
  (1006, 3, 'FIT001', '跳绳', 'PVC 调节款', '器材室 C-01',
   '训练器材', 25, 25, 'AVAILABLE',
   '2026-05-20 09:35:00', '2026-06-01 09:35:00'),
  (1007, 4, 'OTHER001', '计分牌', '手持翻页式', '器材室 D-01',
   '比赛辅助器材，当前维修中', 4, 4, 'MAINTENANCE',
   '2026-05-20 09:36:00', '2026-06-01 09:36:00')
ON DUPLICATE KEY UPDATE
  `category_id` = VALUES(`category_id`),
  `equipment_code` = VALUES(`equipment_code`),
  `equipment_name` = VALUES(`equipment_name`),
  `specification` = VALUES(`specification`),
  `storage_location` = VALUES(`storage_location`),
  `description` = VALUES(`description`),
  `total_stock` = VALUES(`total_stock`),
  `available_stock` = VALUES(`available_stock`),
  `status` = VALUES(`status`),
  `updated_at` = VALUES(`updated_at`);

-- ------------------------------------------------------------------
-- reservation
-- ------------------------------------------------------------------
INSERT INTO `reservation` (
  `id`, `reservation_no`, `student_user_id`, `equipment_id`,
  `reservation_start_at`, `reservation_end_at`, `quantity`, `status`,
  `request_note`, `review_note`, `cancel_reason`, `reviewed_by`, `reviewed_at`,
  `created_at`, `updated_at`
) VALUES
  (2001, 'RES202606010001', 101, 1001,
   '2026-06-03 15:00:00', '2026-06-03 17:00:00', 1, 'PENDING',
   '篮球课后训练使用', NULL, NULL, NULL, NULL,
   '2026-06-01 10:05:00', '2026-06-01 10:05:00'),
  (2002, 'RES202606010002', 102, 1002,
   '2026-06-03 16:00:00', '2026-06-03 18:00:00', 1, 'APPROVED',
   '足球社团活动使用', '库存充足，审核通过', NULL, 1, '2026-06-01 11:00:00',
   '2026-06-01 10:20:00', '2026-06-01 11:00:00'),
  (2003, 'RES202606010003', 103, 1004,
   '2026-06-04 14:00:00', '2026-06-04 16:00:00', 2, 'REJECTED',
   '羽毛球双打练习', '该时段同类器材预约较多，建议改期', NULL, 2, '2026-06-01 11:10:00',
   '2026-06-01 10:30:00', '2026-06-01 11:10:00'),
  (2004, 'RES202606010004', 101, 1003,
   '2026-06-02 19:00:00', '2026-06-02 20:30:00', 1, 'CANCELED',
   '排球课后自练', NULL, '学生临时取消', NULL, NULL,
   '2026-06-01 10:40:00', '2026-06-01 12:00:00'),
  (2005, 'RES202606010005', 102, 1005,
   '2026-06-01 14:00:00', '2026-06-01 16:00:00', 2, 'BORROWED',
   '乒乓球训练使用', '审核通过并已借出', NULL, 1, '2026-06-01 13:00:00',
   '2026-06-01 11:30:00', '2026-06-01 13:30:00'),
  (2006, 'RES202606010006', 103, 1004,
   '2026-05-31 15:00:00', '2026-05-31 17:00:00', 2, 'COMPLETED',
   '羽毛球课程使用', '已归还，流程完成', NULL, 2, '2026-05-31 13:20:00',
   '2026-05-31 12:50:00', '2026-05-31 18:20:00')
ON DUPLICATE KEY UPDATE
  `reservation_no` = VALUES(`reservation_no`),
  `student_user_id` = VALUES(`student_user_id`),
  `equipment_id` = VALUES(`equipment_id`),
  `reservation_start_at` = VALUES(`reservation_start_at`),
  `reservation_end_at` = VALUES(`reservation_end_at`),
  `quantity` = VALUES(`quantity`),
  `status` = VALUES(`status`),
  `request_note` = VALUES(`request_note`),
  `review_note` = VALUES(`review_note`),
  `cancel_reason` = VALUES(`cancel_reason`),
  `reviewed_by` = VALUES(`reviewed_by`),
  `reviewed_at` = VALUES(`reviewed_at`),
  `updated_at` = VALUES(`updated_at`);

-- ------------------------------------------------------------------
-- borrow_record
-- ------------------------------------------------------------------
INSERT INTO `borrow_record` (
  `id`, `borrow_no`, `reservation_id`, `student_user_id`, `equipment_id`,
  `quantity`, `borrowed_by`, `borrowed_at`, `due_at`, `received_by`,
  `returned_at`, `status`, `return_note`, `created_at`, `updated_at`
) VALUES
  (3001, 'BOR202606010001', 2005, 102, 1005,
   2, 1, '2026-06-01 13:30:00', '2026-06-01 18:00:00', NULL,
   NULL, 'BORROWING', NULL, '2026-06-01 13:30:00', '2026-06-01 13:30:00'),
  (3002, 'BOR202605310001', 2006, 103, 1004,
   2, 2, '2026-05-31 13:30:00', '2026-05-31 18:00:00', 2,
   '2026-05-31 17:45:00', 'RETURNED', '器材完好，已正常归还',
   '2026-05-31 13:30:00', '2026-05-31 17:45:00')
ON DUPLICATE KEY UPDATE
  `borrow_no` = VALUES(`borrow_no`),
  `reservation_id` = VALUES(`reservation_id`),
  `student_user_id` = VALUES(`student_user_id`),
  `equipment_id` = VALUES(`equipment_id`),
  `quantity` = VALUES(`quantity`),
  `borrowed_by` = VALUES(`borrowed_by`),
  `borrowed_at` = VALUES(`borrowed_at`),
  `due_at` = VALUES(`due_at`),
  `received_by` = VALUES(`received_by`),
  `returned_at` = VALUES(`returned_at`),
  `status` = VALUES(`status`),
  `return_note` = VALUES(`return_note`),
  `updated_at` = VALUES(`updated_at`);

-- ------------------------------------------------------------------
-- damage_record
-- ------------------------------------------------------------------
INSERT INTO `damage_record` (
  `id`, `borrow_record_id`, `equipment_id`, `responsible_user_id`, `reported_by`,
  `incident_type`, `severity`, `description`, `estimated_cost`, `handling_status`,
  `resolution_note`, `reported_at`, `resolved_at`, `created_at`, `updated_at`
) VALUES
  (4001, 3002, 1004, 103, 2,
   'DAMAGE', 'LOW', '归还时发现拍柄防滑胶轻微磨损，不影响继续使用。', 15.00, 'RESOLVED',
   '已更换防滑胶并记录维护情况。', '2026-05-31 17:50:00', '2026-05-31 18:10:00',
   '2026-05-31 17:50:00', '2026-05-31 18:10:00')
ON DUPLICATE KEY UPDATE
  `borrow_record_id` = VALUES(`borrow_record_id`),
  `equipment_id` = VALUES(`equipment_id`),
  `responsible_user_id` = VALUES(`responsible_user_id`),
  `reported_by` = VALUES(`reported_by`),
  `incident_type` = VALUES(`incident_type`),
  `severity` = VALUES(`severity`),
  `description` = VALUES(`description`),
  `estimated_cost` = VALUES(`estimated_cost`),
  `handling_status` = VALUES(`handling_status`),
  `resolution_note` = VALUES(`resolution_note`),
  `reported_at` = VALUES(`reported_at`),
  `resolved_at` = VALUES(`resolved_at`),
  `updated_at` = VALUES(`updated_at`);

-- ------------------------------------------------------------------
-- operation_log
-- ------------------------------------------------------------------
INSERT INTO `operation_log` (
  `id`, `operator_user_id`, `operator_username`, `operation_type`,
  `target_type`, `target_id`, `operation_result`, `ip_address`,
  `request_id`, `detail`, `created_at`
) VALUES
  (5001, 1, 'admin01', 'LOGIN', 'USER', 1, 'SUCCESS', '127.0.0.1',
   'REQ-20260601-0001', '管理员登录成功。', '2026-06-01 09:00:03'),
  (5002, 101, 'stu2026001', 'CREATE_RESERVATION', 'RESERVATION', 2001, 'SUCCESS', '127.0.0.1',
   'REQ-20260601-0002', '学生创建了一条待审核预约记录。', '2026-06-01 10:05:10'),
  (5003, 1, 'admin01', 'APPROVE_RESERVATION', 'RESERVATION', 2002, 'SUCCESS', '127.0.0.1',
   'REQ-20260601-0003', '管理员审核通过预约 RES202606010002。', '2026-06-01 11:00:05'),
  (5004, 2, 'admin02', 'REJECT_RESERVATION', 'RESERVATION', 2003, 'SUCCESS', '127.0.0.1',
   'REQ-20260601-0004', '管理员拒绝预约 RES202606010003。', '2026-06-01 11:10:08'),
  (5005, 1, 'admin01', 'BORROW_EQUIPMENT', 'BORROW_RECORD', 3001, 'SUCCESS', '127.0.0.1',
   'REQ-20260601-0005', '管理员办理借出，生成借用记录 BOR202606010001。', '2026-06-01 13:30:10'),
  (5006, 2, 'admin02', 'REGISTER_DAMAGE', 'DAMAGE_RECORD', 4001, 'SUCCESS', '127.0.0.1',
   'REQ-20260531-0006', '管理员登记一条轻微损坏记录。', '2026-05-31 17:50:30')
ON DUPLICATE KEY UPDATE
  `operator_user_id` = VALUES(`operator_user_id`),
  `operator_username` = VALUES(`operator_username`),
  `operation_type` = VALUES(`operation_type`),
  `target_type` = VALUES(`target_type`),
  `target_id` = VALUES(`target_id`),
  `operation_result` = VALUES(`operation_result`),
  `ip_address` = VALUES(`ip_address`),
  `request_id` = VALUES(`request_id`),
  `detail` = VALUES(`detail`),
  `created_at` = VALUES(`created_at`);

COMMIT;
