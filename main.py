import os
from google import genai
from dotenv import load_dotenv
import firebase_admin
from firebase_admin import credentials, firestore

flag = 1

# The client automatically picks up the API key from the environment variable
load_dotenv()
api_key=os.getenv("API_KEY")
# Load service account key
cred = credentials.Certificate("serviceAccountKey.json")

firebase_admin.initialize_app(cred)

# Get Firestore client
db = firestore.client()

collections = db.collection("roster").stream()
for doc in collections: #goes through all the people in the roster
    print("Document ID:", doc.id)
    print("Data:", doc.to_dict())
    print("------")
client = genai.Client(api_key=api_key)
if(flag == 0):
    response = client.models.generate_content(
        model="gemini-2.5-flash", # Use an available model, e.g., gemini-2.5-flash
        contents="whats 2+2"
    )
    print(response.text)