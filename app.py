import os
from google import genai
from google.genai import types
from dotenv import load_dotenv
import firebase_admin
from firebase_admin import credentials, firestore, storage
import cv2
from deepface import DeepFace
from flask import Flask, request
import numpy as np
import requests
import time
app = Flask(__name__)
exitflag = 1
flag = 1
imglist = []
# The client automatically picks up the API key from the environment variable
load_dotenv()
api_key=os.getenv("API_KEY")
# Load service account key
cred = credentials.Certificate("serviceAccountKey.json")
client = genai.Client(api_key=api_key)
firebase_admin.initialize_app(cred)

# Get Firestore client
db = firestore.client()

def download_and_save(storage_path, local_path): #get new images
    try:
        # Create local directory if it doesn't exist
        os.makedirs(os.path.dirname(local_path) or ".", exist_ok=True)
        
        bucket = storage.bucket('make-uof-t-project-4li45f.firebasestorage.app')
        blob = bucket.blob(storage_path)
        
        # Download directly to file
        blob.download_to_filename(local_path)
        print(f"Downloaded to {local_path}")
        
        # Load with OpenCV
        img = cv2.imread(local_path)
        return img
        
    except Exception as e:
        print(f"Error: {e}")
        return None


collections = db.collection("roster").stream()
for doc in collections: #goes through all the people in the roster to download image
    #print("Data:", doc.to_dict())

    dat = doc.to_dict()
    print(dat)
    img_url = dat.get('img')
    name = dat.get('name')
    imglist.append(name)
    #print(dat)
    if 'firebasestorage' in img_url:
        # Extract the path between '/o/' and '?'
        start = img_url.find('/o/') + 3
        end = img_url.find('?', start)
        if end == -1:
            end = len(img_url)
        blob_path = img_url[start:end]
    else:
        blob_path = img_url
        
    local_filename = os.path.basename(blob_path)
    pic, ext = os.path.splitext(local_filename)
    newfile = name + ext
    download_and_save(blob_path, f"downloaded_images/{newfile}")

while(exitflag == 0):
    try:
        response = requests.get("http://10.17.114.108/capture", timeout=10)
        
        print(f"Response status: {response.status_code}")
        print(f"Content type: {response.headers.get('Content-Type')}")
        print(f"Content length: {len(response.content)}")
        
        # Save raw response first
        with open('temp.jpg', 'wb') as f:
            f.write(response.content)
        
        # Try to load it
        img = cv2.imread('temp.jpg')
        
        if img is None:
            print("Failed to load image")
            time.sleep(3)
            continue
        
        print(f"Image loaded: {img.shape}")
        
        # Check for face
        try:
            DeepFace.extract_faces(img_path='temp.jpg', enforce_detection=True)
            print("Face detected!")
            exitflag = 1
        except:
            print("No face detected")
            
    except Exception as err:
        print(f"Error: {err}")
    
    time.sleep(3)
# Now you have the image
print('yay')

@app.route('/upload', methods=['POST']) # the single request to go from in to out
def upload_photo():
    image_data = 'temp.jpg'
    
    # Your processing here
    with open(image_data, 'rb') as f:
      image_bytes = f.read()
    result = []
    for x in range(4):
        result[x] = DeepFace.verify(image_data, "downloaded_images/"+imglist[x])
    max_value = max(result)
    max_index = result.index(max_value)
    thej = imglist[max_index] 
    docs = db.collection('roster').where('name', '==', thej).limit(1).stream()
    for dc in docs:
        special = doc.to_dict()
        interests = special['interests']
        print(interests)  # ['Hiking', 'Coffee']
    if(flag == 0):
        response = client.models.generate_content(
        model="gemini-2.5-flash", # Use an available model, e.g., gemini-2.5-flash
        contents=[
            types.Part.from_bytes(
                data=image_bytes,
                mime_type='image/jpeg',
            ),
        "There is a beatiful person named" + thej + " who  can be seen  in the image attatched and is very interested in" + interests + "make 5 pick up lines/conversation starters and provide no additional text other than the pick up lines seperated by comas"]
    )
    print(response.text)

    
    return response.text, 200

@app.route('/poop', methods=['POST'])
def poopy():
    return "poop", 200
if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)