import os
from google import genai
from dotenv import load_dotenv
# The client automatically picks up the API key from the environment variable
load_dotenv()
api_key=os.getenv("API_KEY")

client = genai.Client(api_key=api_key)
response = client.models.generate_content(
    model="gemini-2.5-flash", # Use an available model, e.g., gemini-2.5-flash
    contents="whats 2+2"
)

print(response.text)