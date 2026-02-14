import os
from google import genai
from dotenv import load_dotenv
import firebase_admin
from firebase_admin import credentials, firestore, storage
import cv2
from deepface import DeepFace
flag = 1
imglist = ['Jahangeer.jpg','Jessica.webp','Emily.webp','Lily.jpg']
imgcode = []
# The client automatically picks up the API key from the environment variable
load_dotenv()
api_key=os.getenv("API_KEY")
# Load service account key
cred = credentials.Certificate("serviceAccountKey.json")

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
    img_url = dat.get('img')
    #imglist.append(img_url)
    #print(dat)
    """if 'firebasestorage' in img_url:
        # Extract the path between '/o/' and '?'
        start = img_url.find('/o/') + 3
        end = img_url.find('?', start)
        if end == -1:
            end = len(img_url)
        blob_path = img_url[start:end]
    else:
        blob_path = img_url
    
    local_filename = os.path.basename(blob_path)
    download_and_save(blob_path, f"downloaded_images/{local_filename}")"""

client = genai.Client(api_key=api_key)
if(flag == 0):
    response = client.models.generate_content(
        model="gemini-2.5-flash", # Use an available model, e.g., gemini-2.5-flash
        contents="whats 2+2"
    )
    print(response.text)

for x in range(4):
    result = DeepFace.verify("downloaded_images/Jahangeer.jpg", "downloaded_images/"+imglist[x])
    print(result)

