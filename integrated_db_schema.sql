-- =========================================================
-- INTEGRATED SMART STORE DATABASE SCHEMA
-- Combines fingerprint authentication with visual recognition
-- =========================================================

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- Create database
CREATE DATABASE IF NOT EXISTS smart_store_integrated
  CHARACTER SET utf8mb4
  COLLATE utf8mb4_general_ci;

USE smart_store_integrated;

-- Drop existing tables in correct order
DROP TABLE IF EXISTS transaction_items;
DROP TABLE IF EXISTS transactions;
DROP TABLE IF EXISTS shopping_sessions;
DROP TABLE IF EXISTS fingerprint_logs;
DROP TABLE IF EXISTS clients;
DROP TABLE IF EXISTS products;

-- =========================================================
-- CLIENTS TABLE (Enhanced with biometric data)
-- =========================================================
CREATE TABLE clients (
    client_id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    age INT,
    sex ENUM('M', 'F', 'Other') DEFAULT 'Other',
    fingerprint_id INT UNIQUE NOT NULL,
    face_image_path VARCHAR(255) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    is_active BOOLEAN DEFAULT TRUE,
    
    INDEX idx_fingerprint (fingerprint_id),
    INDEX idx_active (is_active)
) ENGINE=InnoDB;

-- =========================================================
-- PRODUCTS TABLE (Enhanced with images and metadata)
-- =========================================================
CREATE TABLE products (
    product_id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    type VARCHAR(50) NOT NULL,
    price DECIMAL(10,2) NOT NULL CHECK (price >= 0),
    expiry_date DATE,
    image_path VARCHAR(255) NOT NULL,
    visual_signature_id INT UNIQUE NOT NULL,
    stock_quantity INT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    is_active BOOLEAN DEFAULT TRUE,
    
    INDEX idx_visual_signature (visual_signature_id),
    INDEX idx_type (type),
    INDEX idx_active (is_active)
) ENGINE=InnoDB;

-- =========================================================
-- SHOPPING SESSIONS (Track customer shopping in real-time)
-- =========================================================
CREATE TABLE shopping_sessions (
    session_id INT AUTO_INCREMENT PRIMARY KEY,
    client_id INT NOT NULL,
    entry_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    exit_time TIMESTAMP NULL,
    status ENUM('active', 'completed', 'abandoned') DEFAULT 'active',
    face_verified BOOLEAN DEFAULT FALSE,
    fingerprint_verified BOOLEAN DEFAULT FALSE,
    
    FOREIGN KEY (client_id) REFERENCES clients(client_id) 
        ON DELETE CASCADE ON UPDATE CASCADE,
    
    INDEX idx_client (client_id),
    INDEX idx_status (status),
    INDEX idx_entry_time (entry_time)
) ENGINE=InnoDB;

-- =========================================================
-- TRANSACTIONS (Completed purchases)
-- =========================================================
CREATE TABLE transactions (
    transaction_id INT AUTO_INCREMENT PRIMARY KEY,
    session_id INT NOT NULL,
    client_id INT NOT NULL,
    total_amount DECIMAL(10,2) NOT NULL CHECK (total_amount >= 0),
    payment_status ENUM('pending', 'completed', 'failed') DEFAULT 'pending',
    payment_method ENUM('fingerprint', 'card', 'cash') DEFAULT 'fingerprint',
    invoice_path VARCHAR(255),
    transaction_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    FOREIGN KEY (session_id) REFERENCES shopping_sessions(session_id)
        ON DELETE CASCADE ON UPDATE CASCADE,
    FOREIGN KEY (client_id) REFERENCES clients(client_id)
        ON DELETE CASCADE ON UPDATE CASCADE,
    
    INDEX idx_client (client_id),
    INDEX idx_status (payment_status),
    INDEX idx_time (transaction_time)
) ENGINE=InnoDB;

-- =========================================================
-- TRANSACTION ITEMS (Products in each transaction)
-- =========================================================
CREATE TABLE transaction_items (
    item_id INT AUTO_INCREMENT PRIMARY KEY,
    transaction_id INT NOT NULL,
    product_id INT NOT NULL,
    quantity INT DEFAULT 1,
    unit_price DECIMAL(10,2) NOT NULL,
    subtotal DECIMAL(10,2) NOT NULL,
    
    FOREIGN KEY (transaction_id) REFERENCES transactions(transaction_id)
        ON DELETE CASCADE ON UPDATE CASCADE,
    FOREIGN KEY (product_id) REFERENCES products(product_id)
        ON DELETE CASCADE ON UPDATE CASCADE,
    
    INDEX idx_transaction (transaction_id),
    INDEX idx_product (product_id)
) ENGINE=InnoDB;

-- =========================================================
-- FINGERPRINT LOGS (Audit trail for fingerprint access)
-- =========================================================
CREATE TABLE fingerprint_logs (
    log_id INT AUTO_INCREMENT PRIMARY KEY,
    client_id INT,
    fingerprint_id INT NOT NULL,
    action_type ENUM('entry', 'payment', 'verification') NOT NULL,
    success BOOLEAN DEFAULT TRUE,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    FOREIGN KEY (client_id) REFERENCES clients(client_id)
        ON DELETE SET NULL ON UPDATE CASCADE,
    
    INDEX idx_client (client_id),
    INDEX idx_timestamp (timestamp),
    INDEX idx_action (action_type)
) ENGINE=InnoDB;

-- =========================================================
-- SAMPLE DATA
-- =========================================================

-- Sample clients
INSERT INTO clients (client_id, name, age, sex, fingerprint_id, face_image_path) VALUES
(1, 'Alice Dupont', 28, 'F', 1, '../images/clients/1.jpg'),
(2, 'Bob Martin', 35, 'M', 2, '../images/clients/2.jpg'),
(3, 'Charlie Bernard', 42, 'M', 3, '../images/clients/3.jpg');

-- Sample products
INSERT INTO products (product_id, name, type, price, expiry_date, image_path, visual_signature_id, stock_quantity) VALUES
(1, 'Support Telephone', 'Accessoires', 1.50, NULL, '../images/produits/1.jpg', 1, 50),
(101, 'Bouteille Eau', 'Boissons', 2.50, '2026-12-31', '../images/produits/101.jpg', 101, 100),
(102, 'Smartphone XR', 'Electronique', 25.00, NULL, '../images/produits/102.jpg', 102, 20);

-- Sample shopping session
INSERT INTO shopping_sessions (session_id, client_id, status, face_verified, fingerprint_verified) VALUES
(1, 1, 'active', TRUE, FALSE);

SET FOREIGN_KEY_CHECKS = 1;

-- =========================================================
-- USEFUL QUERIES
-- =========================================================

-- View active shopping sessions
CREATE OR REPLACE VIEW v_active_sessions AS
SELECT 
    s.session_id,
    s.client_id,
    c.name as client_name,
    s.entry_time,
    s.face_verified,
    s.fingerprint_verified,
    TIMESTAMPDIFF(MINUTE, s.entry_time, NOW()) as minutes_in_store
FROM shopping_sessions s
JOIN clients c ON s.client_id = c.client_id
WHERE s.status = 'active';

-- View transaction summary
CREATE OR REPLACE VIEW v_transaction_summary AS
SELECT 
    t.transaction_id,
    t.client_id,
    c.name as client_name,
    t.total_amount,
    t.payment_status,
    t.transaction_time,
    COUNT(ti.item_id) as item_count
FROM transactions t
JOIN clients c ON t.client_id = c.client_id
LEFT JOIN transaction_items ti ON t.transaction_id = ti.transaction_id
GROUP BY t.transaction_id;

SELECT '--- DATABASE INITIALIZED SUCCESSFULLY ---' AS Status;