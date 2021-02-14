from flask import Flask, render_template, send_from_directory, jsonify, request
from flaskwebgui import FlaskUI #get the FlaskUI class

import sys

import modbus_tk
import modbus_tk.defines as cst
from modbus_tk import modbus_rtu
import serial

app = Flask(__name__)

logger = modbus_tk.utils.create_logger(name="console", record_format="%(message)s")

ui = FlaskUI(app, 400, 300)

@app.route("/")
def index():
    return render_template('index.html')

@app.route('/time', methods=['GET'])
def get_time():
    values = slave.get_values(name, address, length)
    
    values1 = slave.get_values(name, 10, 8)
    # print(values1)

    return jsonify(list(values))

@app.route('/settime', methods=['POST'])
def post_time():
    d = request.args.to_dict()
    values = [1, int(d['HH']),int(d['MM']),0,int(d['day']),int(d['dd']),int(d['mm']),int(d['yyyy'])]
    slave.set_values(name, 10, values)
    values = slave.get_values(name, 11, 7)

    return jsonify(list(values))

server = modbus_rtu.RtuServer(serial.Serial('COM5', baudrate= 115200, bytesize=8, parity='N', stopbits=1, xonxoff=0))
server.start()
slave_1 = server.add_slave(1)
slave_1.add_block('0', cst.HOLDING_REGISTERS, 0, 100)

slave_id = 1
name = '0'
address = 1
length = 7
slave = server.get_slave(slave_id)

ui.run()