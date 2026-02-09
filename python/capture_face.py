#!/usr/bin/env python3
"""
Face Image Capture Script
Used by C registration system to capture client face photos
"""

import cv2
import sys
import os

def capture_face(client_id, output_path):
    """Capture face image from webcam"""
    
    print(f"  Initializing camera...")
    cap = cv2.VideoCapture(0)
    
    if not cap.isOpened():
        print("  ✗ Cannot open camera")
        return 1
    
    # Load face cascade
    face_cascade = cv2.CascadeClassifier(cv2.data.haarcascades + 'haarcascade_frontalface_default.xml')
    
    print(f"  Position your face in the frame...")
    print(f"  Image will be captured automatically when face is detected")
    
    face_detected = False
    countdown = 30  # 30 frames countdown
    
    while True:
        ret, frame = cap.read()
        if not ret:
            break
        
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        faces = face_cascade.detectMultiScale(gray, 1.3, 5)
        
        # Draw rectangles around faces
        for (x, y, w, h) in faces:
            cv2.rectangle(frame, (x, y), (x+w, y+h), (0, 255, 0), 2)
            
            if not face_detected:
                face_detected = True
                print(f"  ✓ Face detected! Capturing in {countdown} frames...")
        
        # Display frame
        cv2.imshow('Face Capture - Press Q to cancel', frame)
        
        # Countdown and capture
        if face_detected:
            countdown -= 1
            if countdown <= 0:
                # Capture the face
                if len(faces) > 0:
                    x, y, w, h = faces[0]
                    face_img = gray[y:y+h, x:x+w]
                    face_img_resized = cv2.resize(face_img, (200, 200))
                    
                    # Ensure directory exists
                    os.makedirs(os.path.dirname(output_path), exist_ok=True)
                    
                    # Save image
                    cv2.imwrite(output_path, face_img_resized)
                    print(f"  ✓ Face image saved: {output_path}")
                    
                    cap.release()
                    cv2.destroyAllWindows()
                    return 0
        
        # Exit on 'q' key
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
    
    cap.release()
    cv2.destroyAllWindows()
    print("  ✗ Capture cancelled")
    return 1

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: capture_face.py <client_id> <output_path>")
        sys.exit(1)
    
    client_id = sys.argv[1]
    output_path = sys.argv[2]
    
    sys.exit(capture_face(client_id, output_path))