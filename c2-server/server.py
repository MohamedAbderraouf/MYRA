from flask import Flask, request, jsonify
import os
import random
import string
import json
import secrets

app = Flask(__name__)

# File to store UID -> Password mapping
UID_PASSWORD_FILE = "./uid_passwords.json"

def load_uid_passwords():
    if os.path.exists(UID_PASSWORD_FILE):
        with open(UID_PASSWORD_FILE, 'r') as file:
            return json.load(file)
    return {}

def save_uid_passwords(data):
    os.makedirs(os.path.dirname(UID_PASSWORD_FILE), exist_ok=True)
    with open(UID_PASSWORD_FILE, 'w') as file:
        json.dump(data, file, indent=4)

# if its the production env the password should be strong
def generate_password(isProdEnv = False):
    
    if isProdEnv:
        # Generate a random secure password (32 hex characters)
        password = secrets.token_hex(16)

    else :
        # dev env simpler passwords for easy debuging 
        password = ''.join(random.choices(string.digits, k=8))

    return password


@app.route('/getpassword', methods=['POST'])
def getpassword():
    data = request.json
    if not data or 'uid' not in data:
        return "Missing UID", 400
    
    # TODO : validate te uid being received here  
    uid = data['uid']
    uid_passwords = load_uid_passwords()

    if uid in uid_passwords:
        password = uid_passwords[uid]
    else:
        password = generate_password()
        uid_passwords[uid] = password
        save_uid_passwords(uid_passwords)

    return jsonify({"uid": uid, "password": password}), 200

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=80)



