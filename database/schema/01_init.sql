-- Sports equipment reservation and borrow/return management system
-- Initial MySQL 8.0 schema

CREATE DATABASE IF NOT EXISTS `sports_equipment_management`
  DEFAULT CHARACTER SET utf8mb4
  DEFAULT COLLATE utf8mb4_0900_ai_ci;

USE `sports_equipment_management`;

-- ------------------------------------------------------------------
-- Table: user
-- Purpose:
--   Store student and administrator accounts.
-- Allowed values:
--   role   : STUDENT | ADMIN
--   status : ACTIVE  | DISABLED
-- ------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `user` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'Primary key',
  `username` VARCHAR(64) NOT NULL COMMENT 'Login name',
  `password_hash` VARCHAR(255) NOT NULL COMMENT 'Password hash instead of plain text password',
  `real_name` VARCHAR(64) NOT NULL COMMENT 'Display name',
  `role` VARCHAR(20) NOT NULL COMMENT 'STUDENT | ADMIN',
  `student_no` VARCHAR(32) NULL COMMENT 'Student number, only used by student accounts',
  `phone` VARCHAR(20) NULL COMMENT 'Phone number',
  `email` VARCHAR(128) NULL COMMENT 'Email address',
  `status` VARCHAR(20) NOT NULL DEFAULT 'ACTIVE' COMMENT 'ACTIVE | DISABLED',
  `last_login_at` DATETIME NULL COMMENT 'Last successful login time',
  `created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'Creation time',
  `updated_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last update time',
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_user_username` (`username`),
  UNIQUE KEY `uk_user_student_no` (`student_no`),
  UNIQUE KEY `uk_user_email` (`email`),
  KEY `idx_user_role_status` (`role`, `status`),
  CONSTRAINT `chk_user_role`
    CHECK (`role` IN ('STUDENT', 'ADMIN')),
  CONSTRAINT `chk_user_status`
    CHECK (`status` IN ('ACTIVE', 'DISABLED'))
) ENGINE=InnoDB COMMENT='User account table';

-- ------------------------------------------------------------------
-- Table: equipment_category
-- Purpose:
--   Store equipment categories such as ball or racket.
-- Allowed values:
--   status : ACTIVE | DISABLED
-- ------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `equipment_category` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'Primary key',
  `category_code` VARCHAR(32) NOT NULL COMMENT 'Business code such as BALL or RACKET',
  `category_name` VARCHAR(64) NOT NULL COMMENT 'Category display name',
  `description` VARCHAR(255) NULL COMMENT 'Category description',
  `sort_order` INT NOT NULL DEFAULT 0 COMMENT 'Display order',
  `status` VARCHAR(20) NOT NULL DEFAULT 'ACTIVE' COMMENT 'ACTIVE | DISABLED',
  `created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'Creation time',
  `updated_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last update time',
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_equipment_category_code` (`category_code`),
  UNIQUE KEY `uk_equipment_category_name` (`category_name`),
  KEY `idx_equipment_category_status` (`status`, `sort_order`),
  CONSTRAINT `chk_equipment_category_status`
    CHECK (`status` IN ('ACTIVE', 'DISABLED'))
) ENGINE=InnoDB COMMENT='Equipment category table';

-- ------------------------------------------------------------------
-- Table: equipment
-- Purpose:
--   Store equipment master data and stock information.
-- Allowed values:
--   status : AVAILABLE | DISABLED | MAINTENANCE
-- ------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `equipment` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'Primary key',
  `category_id` BIGINT UNSIGNED NOT NULL COMMENT 'FK to equipment_category.id',
  `equipment_code` VARCHAR(32) NOT NULL COMMENT 'Business code such as BKB001',
  `equipment_name` VARCHAR(64) NOT NULL COMMENT 'Equipment display name',
  `specification` VARCHAR(128) NULL COMMENT 'Optional specification or model information',
  `storage_location` VARCHAR(128) NULL COMMENT 'Storage location',
  `description` VARCHAR(255) NULL COMMENT 'Description',
  `total_stock` INT UNSIGNED NOT NULL COMMENT 'Total inventory count',
  `available_stock` INT UNSIGNED NOT NULL COMMENT 'Currently available stock',
  `status` VARCHAR(20) NOT NULL DEFAULT 'AVAILABLE' COMMENT 'AVAILABLE | DISABLED | MAINTENANCE',
  `created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'Creation time',
  `updated_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last update time',
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_equipment_code` (`equipment_code`),
  KEY `idx_equipment_category_status` (`category_id`, `status`),
  KEY `idx_equipment_name` (`equipment_name`),
  CONSTRAINT `fk_equipment_category`
    FOREIGN KEY (`category_id`) REFERENCES `equipment_category` (`id`)
    ON UPDATE RESTRICT
    ON DELETE RESTRICT,
  CONSTRAINT `chk_equipment_status`
    CHECK (`status` IN ('AVAILABLE', 'DISABLED', 'MAINTENANCE')),
  CONSTRAINT `chk_equipment_stock_non_negative`
    CHECK (`total_stock` >= 0 AND `available_stock` >= 0),
  CONSTRAINT `chk_equipment_available_not_exceed_total`
    CHECK (`available_stock` <= `total_stock`)
) ENGINE=InnoDB COMMENT='Equipment master and stock table';

-- ------------------------------------------------------------------
-- Table: reservation
-- Purpose:
--   Store reservation requests and review results.
-- Allowed values:
--   status : PENDING | APPROVED | REJECTED | CANCELED | BORROWED | COMPLETED
-- ------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `reservation` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'Primary key',
  `reservation_no` VARCHAR(32) NOT NULL COMMENT 'Business reservation number',
  `student_user_id` BIGINT UNSIGNED NOT NULL COMMENT 'FK to user.id, must point to a student user',
  `equipment_id` BIGINT UNSIGNED NOT NULL COMMENT 'FK to equipment.id',
  `reservation_start_at` DATETIME NOT NULL COMMENT 'Reservation start time',
  `reservation_end_at` DATETIME NOT NULL COMMENT 'Reservation end time',
  `quantity` INT UNSIGNED NOT NULL COMMENT 'Requested quantity',
  `status` VARCHAR(20) NOT NULL DEFAULT 'PENDING' COMMENT 'PENDING | APPROVED | REJECTED | CANCELED | BORROWED | COMPLETED',
  `request_note` VARCHAR(255) NULL COMMENT 'Optional note entered by student',
  `review_note` VARCHAR(255) NULL COMMENT 'Optional review note entered by admin',
  `cancel_reason` VARCHAR(255) NULL COMMENT 'Reason when reservation is canceled',
  `reviewed_by` BIGINT UNSIGNED NULL COMMENT 'FK to user.id, administrator reviewer',
  `reviewed_at` DATETIME NULL COMMENT 'Review time',
  `created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'Creation time',
  `updated_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last update time',
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_reservation_no` (`reservation_no`),
  KEY `idx_reservation_student_status` (`student_user_id`, `status`, `created_at`),
  KEY `idx_reservation_equipment_status_time` (`equipment_id`, `status`, `reservation_start_at`, `reservation_end_at`),
  KEY `idx_reservation_review_status` (`status`, `reviewed_at`),
  CONSTRAINT `fk_reservation_student_user`
    FOREIGN KEY (`student_user_id`) REFERENCES `user` (`id`)
    ON UPDATE RESTRICT
    ON DELETE RESTRICT,
  CONSTRAINT `fk_reservation_equipment`
    FOREIGN KEY (`equipment_id`) REFERENCES `equipment` (`id`)
    ON UPDATE RESTRICT
    ON DELETE RESTRICT,
  CONSTRAINT `fk_reservation_reviewed_by`
    FOREIGN KEY (`reviewed_by`) REFERENCES `user` (`id`)
    ON UPDATE RESTRICT
    ON DELETE SET NULL,
  CONSTRAINT `chk_reservation_status`
    CHECK (`status` IN ('PENDING', 'APPROVED', 'REJECTED', 'CANCELED', 'BORROWED', 'COMPLETED')),
  CONSTRAINT `chk_reservation_quantity_positive`
    CHECK (`quantity` > 0),
  CONSTRAINT `chk_reservation_time_order`
    CHECK (`reservation_end_at` > `reservation_start_at`)
) ENGINE=InnoDB COMMENT='Reservation request table';

-- ------------------------------------------------------------------
-- Table: borrow_record
-- Purpose:
--   Store actual borrow/return records after reservation approval.
-- Allowed values:
--   status : BORROWING | RETURNED | OVERDUE | DAMAGED_PENDING | CLOSED
-- ------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `borrow_record` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'Primary key',
  `borrow_no` VARCHAR(32) NOT NULL COMMENT 'Business borrow number',
  `reservation_id` BIGINT UNSIGNED NOT NULL COMMENT 'FK to reservation.id, one reservation maps to at most one borrow record',
  `student_user_id` BIGINT UNSIGNED NOT NULL COMMENT 'FK to user.id',
  `equipment_id` BIGINT UNSIGNED NOT NULL COMMENT 'FK to equipment.id',
  `quantity` INT UNSIGNED NOT NULL COMMENT 'Borrowed quantity',
  `borrowed_by` BIGINT UNSIGNED NOT NULL COMMENT 'FK to user.id, administrator who confirmed borrow',
  `borrowed_at` DATETIME NOT NULL COMMENT 'Borrow time',
  `due_at` DATETIME NOT NULL COMMENT 'Expected return deadline',
  `received_by` BIGINT UNSIGNED NULL COMMENT 'FK to user.id, administrator who confirmed return',
  `returned_at` DATETIME NULL COMMENT 'Actual return time',
  `status` VARCHAR(20) NOT NULL DEFAULT 'BORROWING' COMMENT 'BORROWING | RETURNED | OVERDUE | DAMAGED_PENDING | CLOSED',
  `return_note` VARCHAR(255) NULL COMMENT 'Optional return note',
  `created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'Creation time',
  `updated_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last update time',
  PRIMARY KEY (`id`),
  UNIQUE KEY `uk_borrow_record_borrow_no` (`borrow_no`),
  UNIQUE KEY `uk_borrow_record_reservation_id` (`reservation_id`),
  KEY `idx_borrow_record_student_status` (`student_user_id`, `status`, `borrowed_at`),
  KEY `idx_borrow_record_equipment_status` (`equipment_id`, `status`, `borrowed_at`),
  KEY `idx_borrow_record_due_status` (`due_at`, `status`),
  CONSTRAINT `fk_borrow_record_reservation`
    FOREIGN KEY (`reservation_id`) REFERENCES `reservation` (`id`)
    ON UPDATE RESTRICT
    ON DELETE RESTRICT,
  CONSTRAINT `fk_borrow_record_student_user`
    FOREIGN KEY (`student_user_id`) REFERENCES `user` (`id`)
    ON UPDATE RESTRICT
    ON DELETE RESTRICT,
  CONSTRAINT `fk_borrow_record_equipment`
    FOREIGN KEY (`equipment_id`) REFERENCES `equipment` (`id`)
    ON UPDATE RESTRICT
    ON DELETE RESTRICT,
  CONSTRAINT `fk_borrow_record_borrowed_by`
    FOREIGN KEY (`borrowed_by`) REFERENCES `user` (`id`)
    ON UPDATE RESTRICT
    ON DELETE RESTRICT,
  CONSTRAINT `fk_borrow_record_received_by`
    FOREIGN KEY (`received_by`) REFERENCES `user` (`id`)
    ON UPDATE RESTRICT
    ON DELETE SET NULL,
  CONSTRAINT `chk_borrow_record_status`
    CHECK (`status` IN ('BORROWING', 'RETURNED', 'OVERDUE', 'DAMAGED_PENDING', 'CLOSED')),
  CONSTRAINT `chk_borrow_record_quantity_positive`
    CHECK (`quantity` > 0),
  CONSTRAINT `chk_borrow_record_due_after_borrow`
    CHECK (`due_at` > `borrowed_at`),
  CONSTRAINT `chk_borrow_record_return_after_borrow`
    CHECK (`returned_at` IS NULL OR `returned_at` >= `borrowed_at`)
) ENGINE=InnoDB COMMENT='Borrow and return record table';

-- ------------------------------------------------------------------
-- Table: damage_record
-- Purpose:
--   Store abnormal records such as damage, loss, overdue, or other issues.
-- Allowed values:
--   incident_type   : DAMAGE | LOSS | OVERDUE | OTHER
--   severity        : LOW | MEDIUM | HIGH
--   handling_status : OPEN | IN_PROGRESS | RESOLVED | CLOSED
-- ------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `damage_record` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'Primary key',
  `borrow_record_id` BIGINT UNSIGNED NOT NULL COMMENT 'FK to borrow_record.id',
  `equipment_id` BIGINT UNSIGNED NOT NULL COMMENT 'FK to equipment.id',
  `responsible_user_id` BIGINT UNSIGNED NULL COMMENT 'FK to user.id, usually the borrowing student',
  `reported_by` BIGINT UNSIGNED NOT NULL COMMENT 'FK to user.id, administrator reporter',
  `incident_type` VARCHAR(20) NOT NULL COMMENT 'DAMAGE | LOSS | OVERDUE | OTHER',
  `severity` VARCHAR(20) NOT NULL DEFAULT 'LOW' COMMENT 'LOW | MEDIUM | HIGH',
  `description` TEXT NOT NULL COMMENT 'Issue description',
  `estimated_cost` DECIMAL(10,2) NULL COMMENT 'Optional estimated loss or repair cost',
  `handling_status` VARCHAR(20) NOT NULL DEFAULT 'OPEN' COMMENT 'OPEN | IN_PROGRESS | RESOLVED | CLOSED',
  `resolution_note` VARCHAR(255) NULL COMMENT 'Resolution note',
  `reported_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'Issue report time',
  `resolved_at` DATETIME NULL COMMENT 'Issue resolution time',
  `created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'Creation time',
  `updated_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT 'Last update time',
  PRIMARY KEY (`id`),
  KEY `idx_damage_record_borrow` (`borrow_record_id`),
  KEY `idx_damage_record_equipment` (`equipment_id`),
  KEY `idx_damage_record_type_status` (`incident_type`, `handling_status`, `reported_at`),
  KEY `idx_damage_record_responsible_user` (`responsible_user_id`, `handling_status`),
  CONSTRAINT `fk_damage_record_borrow`
    FOREIGN KEY (`borrow_record_id`) REFERENCES `borrow_record` (`id`)
    ON UPDATE RESTRICT
    ON DELETE RESTRICT,
  CONSTRAINT `fk_damage_record_equipment`
    FOREIGN KEY (`equipment_id`) REFERENCES `equipment` (`id`)
    ON UPDATE RESTRICT
    ON DELETE RESTRICT,
  CONSTRAINT `fk_damage_record_responsible_user`
    FOREIGN KEY (`responsible_user_id`) REFERENCES `user` (`id`)
    ON UPDATE RESTRICT
    ON DELETE SET NULL,
  CONSTRAINT `fk_damage_record_reported_by`
    FOREIGN KEY (`reported_by`) REFERENCES `user` (`id`)
    ON UPDATE RESTRICT
    ON DELETE RESTRICT,
  CONSTRAINT `chk_damage_record_incident_type`
    CHECK (`incident_type` IN ('DAMAGE', 'LOSS', 'OVERDUE', 'OTHER')),
  CONSTRAINT `chk_damage_record_severity`
    CHECK (`severity` IN ('LOW', 'MEDIUM', 'HIGH')),
  CONSTRAINT `chk_damage_record_handling_status`
    CHECK (`handling_status` IN ('OPEN', 'IN_PROGRESS', 'RESOLVED', 'CLOSED')),
  CONSTRAINT `chk_damage_record_estimated_cost`
    CHECK (`estimated_cost` IS NULL OR `estimated_cost` >= 0),
  CONSTRAINT `chk_damage_record_resolved_time`
    CHECK (`resolved_at` IS NULL OR `resolved_at` >= `reported_at`)
) ENGINE=InnoDB COMMENT='Damage and abnormal issue record table';

-- ------------------------------------------------------------------
-- Table: operation_log
-- Purpose:
--   Store key operation logs for audit and tracing.
-- Allowed values:
--   operation_result : SUCCESS | FAILURE
-- ------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `operation_log` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'Primary key',
  `operator_user_id` BIGINT UNSIGNED NULL COMMENT 'FK to user.id, nullable for system or pre-login failures',
  `operator_username` VARCHAR(64) NULL COMMENT 'Operator username snapshot',
  `operation_type` VARCHAR(64) NOT NULL COMMENT 'Operation type such as LOGIN or APPROVE_RESERVATION',
  `target_type` VARCHAR(64) NULL COMMENT 'Business target type such as EQUIPMENT or RESERVATION',
  `target_id` BIGINT UNSIGNED NULL COMMENT 'Business target primary key value',
  `operation_result` VARCHAR(20) NOT NULL DEFAULT 'SUCCESS' COMMENT 'SUCCESS | FAILURE',
  `ip_address` VARCHAR(45) NULL COMMENT 'IPv4 or IPv6 address',
  `request_id` VARCHAR(64) NULL COMMENT 'Optional request correlation id',
  `detail` TEXT NULL COMMENT 'Detailed operation payload or summary',
  `created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'Log creation time',
  PRIMARY KEY (`id`),
  KEY `idx_operation_log_operator_time` (`operator_user_id`, `created_at`),
  KEY `idx_operation_log_type_time` (`operation_type`, `created_at`),
  KEY `idx_operation_log_target` (`target_type`, `target_id`, `created_at`),
  CONSTRAINT `fk_operation_log_operator_user`
    FOREIGN KEY (`operator_user_id`) REFERENCES `user` (`id`)
    ON UPDATE RESTRICT
    ON DELETE SET NULL,
  CONSTRAINT `chk_operation_log_result`
    CHECK (`operation_result` IN ('SUCCESS', 'FAILURE'))
) ENGINE=InnoDB COMMENT='Operation audit log table';
