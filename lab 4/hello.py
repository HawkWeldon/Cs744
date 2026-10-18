from flask import Flask, request
app = Flask(__name__)

@app.route('/hello')
def helloname():
    name = request.args.get('name')
    return f"Hello {name}!!!"

if __name__ == '__main__':
    app.run()