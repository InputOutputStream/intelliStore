#!/usr/bin/env python3
"""
INTEGRATED SMART STORE SYSTEM
Combines face recognition, product tracking, and fingerprint payment
"""

import cv2
import requests
import mysql.connector
import os
import time
from datetime import datetime
from fpdf import FPDF
import numpy as np

# ==================== CONFIGURATION ====================

DB_CONFIG = {
    'user': 'smart_admin',
    'password': 'MonMotDePasse123!',
    'host': 'localhost',
    'database': 'smart_store_integrated'
}

PATHS = {
    'invoices': '../invoices',
    'temp_faces': '../images/temp',
    'temp_products': '../images/temp_produit',
    'cascade': cv2.data.haarcascades + 'haarcascade_frontalface_default.xml'
}

API_URLS = {
    'face_recognition': 'http://localhost:8000/identify',
    'product_recognition': 'http://localhost:8080/identify_produit',
    'fingerprint_api': 'http://localhost:5000/api/identify'
}

# ==================== DATABASE MANAGER ====================

class DatabaseManager:
    """Enhanced database manager for integrated system"""
    
    def __init__(self, config):
        try:
            self.db = mysql.connector.connect(**config)
            self.cursor = self.db.cursor(dictionary=True)
            print("✓ Database connected")
        except mysql.connector.Error as e:
            print(f"✗ Database connection error: {e}")
            exit(1)
    
    def execute_query(self, query, params=(), commit=False):
        try:
            self.cursor.execute(query, params)
            if commit:
                self.db.commit()
                return None
            return self.cursor.fetchall()
        except mysql.connector.Error as e:
            print(f"✗ SQL Error: {e}")
            return None
    
    def get_client_by_fingerprint(self, fingerprint_id):
        """Get client by fingerprint ID"""
        query = "SELECT * FROM clients WHERE fingerprint_id = %s AND is_active = 1"
        result = self.execute_query(query, (fingerprint_id,))
        return result[0] if result else None
    
    def create_shopping_session(self, client_id):
        """Create new shopping session"""
        query = "INSERT INTO shopping_sessions (client_id) VALUES (%s)"
        self.execute_query(query, (client_id,), commit=True)
        return self.cursor.lastrowid
    
    def get_active_session(self, client_id):
        """Get active shopping session for client"""
        query = """
            SELECT * FROM shopping_sessions 
            WHERE client_id = %s AND status = 'active'
            ORDER BY entry_time DESC LIMIT 1
        """
        result = self.execute_query(query, (client_id,))
        return result[0] if result else None
    
    def add_product_to_session(self, session_id, product_id):
        """Track product pickup (temporary storage)"""
        # In real implementation, use a session_items table
        print(f"  → Product {product_id} added to session {session_id}")
    
    def get_product_by_visual_id(self, visual_id):
        """Get product by visual recognition ID"""
        query = "SELECT * FROM products WHERE visual_signature_id = %s AND is_active = 1"
        result = self.execute_query(query, (visual_id,))
        return result[0] if result else None
    
    def verify_face_match(self, session_id, verified):
        """Update face verification status"""
        query = "UPDATE shopping_sessions SET face_verified = %s WHERE session_id = %s"
        self.execute_query(query, (verified, session_id), commit=True)
    
    def verify_fingerprint_match(self, session_id, verified):
        """Update fingerprint verification status"""
        query = "UPDATE shopping_sessions SET fingerprint_verified = %s WHERE session_id = %s"
        self.execute_query(query, (verified, session_id), commit=True)
    
    def get_session_items(self, session_id):
        """Get all items in shopping session (mock for now)"""
        # In real system, query session_items table
        # For now, return sample data
        query = """
            SELECT p.product_id, p.name, p.price, 1 as quantity
            FROM products p
            LIMIT 3
        """
        return self.execute_query(query)
    
    def create_transaction(self, session_id, client_id, total_amount):
        """Create transaction record"""
        query = """
            INSERT INTO transactions (session_id, client_id, total_amount, payment_method)
            VALUES (%s, %s, %s, 'fingerprint')
        """
        self.execute_query(query, (session_id, client_id, total_amount), commit=True)
        return self.cursor.lastrowid
    
    def add_transaction_items(self, transaction_id, items):
        """Add items to transaction"""
        query = """
            INSERT INTO transaction_items (transaction_id, product_id, quantity, unit_price, subtotal)
            VALUES (%s, %s, %s, %s, %s)
        """
        for item in items:
            subtotal = item['quantity'] * item['price']
            self.execute_query(query, 
                             (transaction_id, item['product_id'], item['quantity'], 
                              item['price'], subtotal), 
                             commit=True)
    
    def complete_transaction(self, transaction_id, invoice_path):
        """Mark transaction as completed"""
        query = """
            UPDATE transactions 
            SET payment_status = 'completed', invoice_path = %s
            WHERE transaction_id = %s
        """
        self.execute_query(query, (invoice_path, transaction_id), commit=True)
    
    def close_session(self, session_id):
        """Close shopping session"""
        query = """
            UPDATE shopping_sessions 
            SET status = 'completed', exit_time = NOW()
            WHERE session_id = %s
        """
        self.execute_query(query, (session_id,), commit=True)
    
    def log_fingerprint_action(self, client_id, fingerprint_id, action_type, success):
        """Log fingerprint authentication"""
        query = """
            INSERT INTO fingerprint_logs (client_id, fingerprint_id, action_type, success)
            VALUES (%s, %s, %s, %s)
        """
        self.execute_query(query, (client_id, fingerprint_id, action_type, success), commit=True)
    
    def close(self):
        self.cursor.close()
        self.db.close()

# ==================== INVOICE GENERATOR ====================

class InvoiceGenerator:
    """Generate PDF invoices for completed transactions"""
    
    @staticmethod
    def generate(client_name, client_id, items, total, transaction_id):
        pdf = FPDF()
        pdf.add_page()
        
        # Header
        pdf.set_font("Arial", 'B', 24)
        pdf.cell(190, 15, "SMART STORE", ln=True, align='C')
        pdf.set_font("Arial", '', 12)
        pdf.cell(190, 8, "Automated Shopping Experience", ln=True, align='C')
        pdf.ln(10)
        
        # Transaction Info
        pdf.set_font("Arial", 'B', 14)
        pdf.cell(190, 10, "INVOICE", ln=True)
        pdf.set_font("Arial", '', 11)
        pdf.cell(190, 7, f"Transaction ID: {transaction_id}", ln=True)
        pdf.cell(190, 7, f"Date: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}", ln=True)
        pdf.ln(5)
        
        # Client Info
        pdf.set_font("Arial", 'B', 12)
        pdf.cell(190, 8, "Client Information:", ln=True)
        pdf.set_font("Arial", '', 11)
        pdf.cell(190, 7, f"Name: {client_name}", ln=True)
        pdf.cell(190, 7, f"Client ID: {client_id}", ln=True)
        pdf.ln(5)
        
        # Items header
        pdf.set_font("Arial", 'B', 11)
        pdf.cell(90, 8, "Item", border=1)
        pdf.cell(30, 8, "Qty", border=1, align='C')
        pdf.cell(35, 8, "Unit Price", border=1, align='C')
        pdf.cell(35, 8, "Subtotal", border=1, align='C')
        pdf.ln()
        
        # Items
        pdf.set_font("Arial", '', 10)
        for item in items:
            quantity = item.get('quantity', 1)
            price = item['price']
            subtotal = quantity * price
            
            pdf.cell(90, 7, item['name'][:35], border=1)
            pdf.cell(30, 7, str(quantity), border=1, align='C')
            pdf.cell(35, 7, f"{price:.2f} EUR", border=1, align='C')
            pdf.cell(35, 7, f"{subtotal:.2f} EUR", border=1, align='C')
            pdf.ln()
        
        # Total
        pdf.ln(5)
        pdf.set_font("Arial", 'B', 14)
        pdf.cell(155, 10, "TOTAL", align='R')
        pdf.cell(35, 10, f"{total:.2f} EUR", align='C')
        pdf.ln(15)
        
        # Footer
        pdf.set_font("Arial", 'I', 9)
        pdf.cell(190, 7, "Payment verified via biometric authentication", ln=True, align='C')
        pdf.cell(190, 7, "Thank you for shopping with Smart Store!", ln=True, align='C')
        
        # Save
        if not os.path.exists(PATHS['invoices']):
            os.makedirs(PATHS['invoices'])
        
        filename = f"{PATHS['invoices']}/invoice_{transaction_id}_{int(time.time())}.pdf"
        pdf.output(filename)
        return filename

# ==================== VISION RECOGNITION ====================

class VisionRecognition:
    """Interface to C++ vision recognition servers"""
    
    @staticmethod
    def identify_face(image_path):
        """Send face image to C++ server for identification"""
        try:
            response = requests.post(
                API_URLS['face_recognition'],
                data={'path': image_path},
                timeout=2.0
            )
            if response.status_code == 200:
                return response.json().get('client_id')
        except Exception as e:
            print(f"  ⚠ Face recognition error: {e}")
        return None
    
    @staticmethod
    def identify_product(image_path):
        """Send product image to C++ server for identification"""
        try:
            response = requests.post(
                API_URLS['product_recognition'],
                data={'path': image_path},
                timeout=2.0
            )
            if response.status_code == 200:
                data = response.json()
                return data.get('produit_id'), data.get('confidence', 0)
        except Exception as e:
            print(f"  ⚠ Product recognition error: {e}")
        return None, 0

# ==================== FINGERPRINT INTERFACE ====================

class FingerprintInterface:
    """Interface to fingerprint system API"""
    
    @staticmethod
    def identify_client():
        """Wait for fingerprint scan and identify client"""
        try:
            # In real system, this would wait for fingerprint scan
            # For now, simulate API call
            print("  → Waiting for fingerprint scan...")
            time.sleep(2)
            
            # Mock response - in real system, C program sends POST to API
            # We would poll or listen for that event
            return None  # Would return fingerprint_id
            
        except Exception as e:
            print(f"  ✗ Fingerprint error: {e}")
        return None

# ==================== MAIN SMART STORE SYSTEM ====================

class SmartStoreSystem:
    """Main integrated smart store system"""
    
    def __init__(self):
        self.db = DatabaseManager(DB_CONFIG)
        self.vision = VisionRecognition()
        self.fingerprint = FingerprintInterface()
        self.face_cascade = cv2.CascadeClassifier(PATHS['cascade'])
        
        # Tracking state
        self.active_sessions = {}  # {client_id: session_data}
        self.product_detections = {}  # Anti-bounce
        self.last_face_check = {}
        
        # Create temp directories
        for path in PATHS.values():
            if 'temp' in path and not os.path.exists(path):
                os.makedirs(path)
    
    def detect_and_identify_client(self, frame, gray):
        """Detect face and identify client"""
        faces = self.face_cascade.detectMultiScale(gray, 1.3, 5)
        
        current_clients = []
        
        for (x, y, w, h) in faces:
            # Extract face ROI
            face_img = gray[y:y+h, x:x+w]
            face_img_resized = cv2.resize(face_img, (200, 200))
            
            # Save temporarily
            img_path = os.path.join(PATHS['temp_faces'], "current_face.jpg")
            cv2.imwrite(img_path, face_img_resized)
            
            # Identify via C++ server
            client_id = self.vision.identify_face(img_path)
            
            if client_id:
                current_clients.append(client_id)
                
                # Draw green rectangle for recognized client
                cv2.rectangle(frame, (x, y), (x+w, y+h), (0, 255, 0), 2)
                cv2.putText(frame, f"Client ID: {client_id}", 
                           (x, y-10), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
                
                # Create or retrieve session
                if client_id not in self.active_sessions:
                    self.start_shopping_session(client_id)
                
                # Verify face periodically
                now = time.time()
                if client_id not in self.last_face_check or \
                   (now - self.last_face_check[client_id]) > 5:
                    session = self.active_sessions[client_id]
                    self.db.verify_face_match(session['session_id'], True)
                    self.last_face_check[client_id] = now
                    
            else:
                # Unknown face
                cv2.rectangle(frame, (x, y), (x+w, y+h), (0, 0, 255), 2)
                cv2.putText(frame, "Unknown", 
                           (x, y-10), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)
        
        return current_clients
    
    def detect_and_identify_products(self, frame, current_clients):
        """Detect products in scanning zone"""
        h_frame, w_frame, _ = frame.shape
        
        # Define scanning zone (top-left corner)
        box_size = 250
        margin = 20
        x1, y1 = margin, margin
        x2, y2 = x1 + box_size, y1 + box_size
        
        # Draw scanning zone
        cv2.rectangle(frame, (x1, y1), (x2, y2), (255, 0, 0), 3)
        cv2.putText(frame, "PRODUCT SCAN ZONE", 
                   (x1, y1 - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 0, 0), 2)
        
        # Only scan if client is present
        if current_clients:
            client_id = current_clients[0]  # Use first detected client
            
            # Extract product ROI
            roi = frame[y1:y2, x1:x2]
            prod_path = os.path.join(PATHS['temp_products'], "scan_product.jpg")
            cv2.imwrite(prod_path, roi)
            
            # Identify product
            product_id, confidence = self.vision.identify_product(prod_path)
            
            if product_id and confidence > 0:
                # Anti-bounce: require consistent detection
                key = (client_id, product_id)
                
                if key not in self.product_detections:
                    self.product_detections[key] = {'count': 1, 'last_seen': time.time()}
                else:
                    self.product_detections[key]['count'] += 1
                    self.product_detections[key]['last_seen'] = time.time()
                
                # Add to cart after 15 consistent detections
                if self.product_detections[key]['count'] == 15:
                    self.add_product_to_cart(client_id, product_id)
                    cv2.putText(frame, f"ADDED: Product {product_id}", 
                               (x1, y2 + 30), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2)
                    self.product_detections[key]['count'] = -50  # Cooldown
                
                # Show detection status
                cv2.putText(frame, f"Detecting: {product_id} ({confidence:.1f})", 
                           (x1, y2 + 60), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 0), 2)
            
            # Clean old detections
            now = time.time()
            to_remove = [k for k, v in self.product_detections.items() 
                        if (now - v['last_seen']) > 2]
            for k in to_remove:
                del self.product_detections[k]
    
    def start_shopping_session(self, client_id):
        """Initialize shopping session for client"""
        session_id = self.db.create_shopping_session(client_id)
        
        self.active_sessions[client_id] = {
            'session_id': session_id,
            'products': [],
            'entry_time': datetime.now(),
            'face_verified': False,
            'fingerprint_verified': False
        }
        
        print(f"\n🛒 Shopping session started for Client {client_id}")
        print(f"   Session ID: {session_id}")
    
    def add_product_to_cart(self, client_id, product_id):
        """Add product to client's cart"""
        if client_id in self.active_sessions:
            session = self.active_sessions[client_id]
            
            # Get product details
            product = self.db.get_product_by_visual_id(product_id)
            
            if product:
                session['products'].append(product)
                self.db.add_product_to_session(session['session_id'], product['product_id'])
                
                print(f"  ✓ Added: {product['name']} (€{product['price']})")
    
    def process_payment(self, client_id):
        """Process payment via fingerprint"""
        print(f"\n{'='*60}")
        print(f"  PAYMENT PROCESSING - CLIENT {client_id}")
        print(f"{'='*60}\n")
        
        if client_id not in self.active_sessions:
            print("  ✗ No active session found")
            return False
        
        session = self.active_sessions[client_id]
        
        # Calculate total
        items = session['products']
        total = sum(item['price'] for item in items)
        
        if not items:
            print("  ⚠ Cart is empty")
            return False
        
        # Display cart
        print("  Cart Contents:")
        for item in items:
            print(f"    • {item['name']}: €{item['price']:.2f}")
        print(f"\n  TOTAL: €{total:.2f}\n")
        
        # Wait for fingerprint
        print("  ┌────────────────────────────────────────┐")
        print("  │  Please place finger on sensor...     │")
        print("  └────────────────────────────────────────┘\n")
        
        # Simulate fingerprint scan (in real system, wait for C program event)
        input("  Press Enter to simulate fingerprint scan...")
        
        # Verify identity
        client_data = self.db.get_client_by_fingerprint(client_id)  # Mock
        
        if not client_data:
            print("  ✗ Fingerprint not recognized")
            return False
        
        # Log fingerprint action
        self.db.log_fingerprint_action(
            client_id, 
            client_data['fingerprint_id'], 
            'payment', 
            True
        )
        
        # Verify face + fingerprint match
        self.db.verify_fingerprint_match(session['session_id'], True)
        
        # Create transaction
        transaction_id = self.db.create_transaction(
            session['session_id'],
            client_id,
            total
        )
        
        # Add transaction items
        self.db.add_transaction_items(transaction_id, items)
        
        # Generate invoice
        invoice_path = InvoiceGenerator.generate(
            client_data['name'],
            client_id,
            items,
            total,
            transaction_id
        )
        
        # Complete transaction
        self.db.complete_transaction(transaction_id, invoice_path)
        self.db.close_session(session['session_id'])
        
        # Clear session
        del self.active_sessions[client_id]
        
        print(f"\n  ✓ PAYMENT SUCCESSFUL")
        print(f"  Transaction ID: {transaction_id}")
        print(f"  Invoice: {invoice_path}\n")
        print(f"{'='*60}\n")
        
        return True
    
    def run(self):
        """Main system loop"""
        print("\n" + "="*60)
        print("  SMART STORE INTEGRATED SYSTEM")
        print("="*60 + "\n")
        print("  Features:")
        print("  • Automatic face recognition")
        print("  • Real-time product tracking")
        print("  • Biometric payment authentication\n")
        print("  Controls:")
        print("  • 'p' - Process Payment")
        print("  • 'q' - Quit\n")
        print("="*60 + "\n")
        
        cap = cv2.VideoCapture(0)
        
        if not cap.isOpened():
            print("✗ Cannot open camera")
            return
        
        while True:
            ret, frame = cap.read()
            if not ret:
                break
            
            gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
            
            # Step 1: Detect and identify clients
            current_clients = self.detect_and_identify_client(frame, gray)
            
            # Step 2: Detect and track products
            self.detect_and_identify_products(frame, current_clients)
            
            # Display info overlay
            y_offset = frame.shape[0] - 100
            if current_clients:
                for i, client_id in enumerate(current_clients):
                    if client_id in self.active_sessions:
                        session = self.active_sessions[client_id]
                        item_count = len(session['products'])
                        total = sum(p['price'] for p in session['products'])
                        
                        info_text = f"Client {client_id}: {item_count} items | €{total:.2f}"
                        cv2.putText(frame, info_text, 
                                   (10, y_offset + i*30), 
                                   cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
            
            cv2.imshow('Smart Store System', frame)
            
            # Handle keyboard input
            key = cv2.waitKey(1) & 0xFF
            
            if key == ord('q'):
                break
            elif key == ord('p') and current_clients:
                # Process payment for first detected client
                self.process_payment(current_clients[0])
        
        cap.release()
        cv2.destroyAllWindows()
        self.db.close()
        
        print("\n✓ System shutdown complete\n")

# ==================== MAIN ====================

if __name__ == "__main__":
    try:
        system = SmartStoreSystem()
        system.run()
    except KeyboardInterrupt:
        print("\n\n✓ System interrupted by user\n")
    except Exception as e:
        print(f"\n✗ System error: {e}\n")