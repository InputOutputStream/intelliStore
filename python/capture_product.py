#!/usr/bin/env python3
"""
Product Image Capture Script
Used by C registration system to capture product photos
"""

import cv2
import sys
import os
import time

def capture_product(product_id, output_path):
    """Capture product image from webcam"""
    
    print(f"  Initializing camera...")
    cap = cv2.VideoCapture(0)
    
    if not cap.isOpened():
        print("  ✗ Cannot open camera")
        return 1
    
    print(f"  Position the product in the center of the frame...")
    print(f"  Image will be captured in 3 seconds...")
    
    # Display countdown
    for i in range(3, 0, -1):
        ret, frame = cap.read()
        if not ret:
            break
        
        # Draw capture zone
        h, w = frame.shape[:2]
        box_size = 250
        x1, y1 = (w - box_size) // 2, (h - box_size) // 2
        x2, y2 = x1 + box_size, y1 + box_size
        
        cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 3)
        
        # Countdown text
        cv2.putText(frame, f"Capturing in {i}...", 
                   (w//2 - 100, h//2), 
                   cv2.FONT_HERSHEY_SIMPLEX, 1.5, (0, 255, 0), 3)
        
        cv2.imshow('Product Capture', frame)
        cv2.waitKey(1000)  # 1 second delay
    
    # Capture final image
    ret, frame = cap.read()
    if ret:
        # Extract center region
        h, w = frame.shape[:2]
        box_size = 250
        x1, y1 = (w - box_size) // 2, (h - box_size) // 2
        x2, y2 = x1 + box_size, y1 + box_size
        
        product_roi = frame[y1:y2, x1:x2]
        
        # Ensure directory exists
        os.makedirs(os.path.dirname(output_path), exist_ok=True)
        
        # Save full color image
        cv2.imwrite(output_path, product_roi)
        print(f"  ✓ Product image saved: {output_path}")
        
        # Show success message
        cv2.putText(frame, "CAPTURED!", 
                   (w//2 - 100, h//2), 
                   cv2.FONT_HERSHEY_SIMPLEX, 1.5, (0, 255, 0), 3)
        cv2.imshow('Product Capture', frame)
        cv2.waitKey(2000)
        
        cap.release()
        cv2.destroyAllWindows()
        return 0
    
    cap.release()
    cv2.destroyAllWindows()
    print("  ✗ Capture failed")
    return 1

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: capture_product.py <product_id> <output_path>")
        sys.exit(1)
    
    product_id = sys.argv[1]
    output_path = sys.argv[2]
    
    sys.exit(capture_product(product_id, output_path))