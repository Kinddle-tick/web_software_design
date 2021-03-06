import datetime
import os
import random
import re
import socket
import sys
# import time
import pyqtgraph as pg
import numpy as np
# from pyqtgraph.Qt import QtCore, QtGui
# from PyQt5 import QtWidgets
import time
from PyQt5 import QtCore, QtGui, QtWidgets
import copy
from HP_GUI import ChatWindow

# from validation import validation

client_path = "cmake-build-debug/S_client"

# pg.setConfigOptions(antialias=True)  # 抗锯齿
GUI_MSG = 0x00.to_bytes(1, "little")
GUI_FILE = 0x01.to_bytes(1, "little")
GUI_SET_NAME_AND_PASSWORD = 0x02.to_bytes(1, "little")
GUI_CHANGE_NAME_AND_PASSWORD = 0x03.to_bytes(1, "little")
USERNAME_LENGTH = 30


def make_package(data:tuple,method:str)->bytes:
    if type(data) != tuple:
        data = tuple(data)
    if method.lower() in ["msg", 'm']:
        if type(data[0])==str:
            return GUI_MSG+data[0].encode()
        elif type(data[0])==bytes:
            return GUI_MSG + data[0]
        else:
            return GUI_MSG
    elif method.lower() in ["file", 'f']:
        if type(data[0])==str:
            return GUI_FILE+data[0].encode()
        elif type(data[0])==bytes:
            return GUI_FILE + data[0]
        else:
            return GUI_FILE
    elif method.lower() in ["setinfo", "set", "si", "snp", "s"]:
        username_byte = bytearray(USERNAME_LENGTH)
        for i in range(len(data[0])):
            username_byte[i] = data[0][i]
        password_byte = bytes.fromhex(hex(data[1] % (2 ** 32))[2:].rjust(8, '0'))
        return bytes(GUI_SET_NAME_AND_PASSWORD+username_byte+password_byte)
    elif method.lower() in ["changeinfo", "change", "cha", "ci", "cnp","c"]:
        username_byte = bytearray(USERNAME_LENGTH)
        for i in range(len(data[0])):
            username_byte[i] = data[0][i]
        password_byte = bytes.fromhex(hex(data[1] % (2 ** 32))[2:].rjust(8, '0'))
        return bytes(GUI_CHANGE_NAME_AND_PASSWORD+username_byte+password_byte)
    return b''

class MySocketThread(QtCore.QThread):
    # 开辟一个线程用来收发消息到确定的的地址，只维护一个线程
    qt_lock = QtCore.QMutex()
    socket_ = None
    sur_addr = None
    status = 0b0000  # socket:0b1--- sur_addr:0b-1-- TCP/UDP:0b--0-/0b--1- ready(tcp):0b---1
    stop_signal = QtCore.pyqtSignal()
    send_recv_signal = QtCore.pyqtSignal(bytes)  # 将收到的内容发给窗口用 需要在窗口中定义相关的处理函数 并与之connect
    stop_flag = False

    def quick_set(self, proto: str, addr):
        self.set_addr(addr)
        if proto == "TCP":
            self.set_socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sur_connect()
        elif proto == "UDP":
            self.set_socket(socket.AF_INET, socket.SOCK_DGRAM)
        else:
            return False

    def set_socket(self, family=socket.AF_INET, type=socket.SOCK_STREAM, proto=-1):
        if self.socket_ is not None:
            self.socket_.close()
            self.status &= 0b1110  # ready肯定是无了置为0
        if type == socket.SOCK_DGRAM:
            self.status |= 0b0011
        elif type == socket.SOCK_STREAM:
            self.status &= 0b1101
        self.socket_ = socket.socket(family, type, proto)  # 一般来说只考虑了TCP和UDP
        self.status |= 0b1000

    def set_addr(self, addr: (str, int)):
        self.status |= 0b0100
        self.sur_addr = addr

    def sur_connect(self):
        if self.status == 0b1101:
            self.set_socket()
            self.socket_.connect(self.sur_addr)

        if self.status == 0b1100:  # TCP且创建了socket和准备了addr
            self.socket_.connect(self.sur_addr)
            self.status |= 0b0001

        elif self.status | 0b0001 == 0b1111:  # UDP
            print("Warning: UDP do not need connect")
        else:
            print("Error: connect failed")

    def signal_cmd(self, command: str):  # 发送给该线程的指令
        if command.lower() in ["stop",'exit']:
            self.stop()
        else:
            print("Invalid command"+command)

    def signal_sendto(self, data: bytes):  # 需要协助发送的内容
        self.socket_.sendto(data, self.sur_addr)

    def recv(self):
        recv_data = b""
        if self.status == 0b1101:  # TCP
            recv_data = self.socket_.recv(1024)
        elif self.status == 0b1111:  # UDP
            recv_data, addr = self.socket_.recvfrom(1024)
        return recv_data

    def __del__(self):
        print(f"exit socket: {self}")

    def run(self):
        if not self.status | 0b0010 == 0b1111:
            return

        self.stop_flag = False
        num = 0
        self.qt_lock.tryLock()
        self.qt_lock.unlock()
        while not self.stop_flag:
            self.qt_lock.lock()
            try:
                recv_data = self.recv()
            except OSError as z:
                print(z)
                self.qt_lock.unlock()
                continue
            self.qt_lock.unlock()
            self.send_recv_signal.emit(recv_data)
            num += 1
        print("a socket exited")
        self.stop_signal.emit()
        self.stop()

    def stop(self):
        if self.stop_flag:
            return
        self.stop_flag = True
        if self.socket_ is not None:
            self.socket_.close()
        self.exit(0)


class MyWidget(QtWidgets.QWidget):

    # region //许多变量的初始化
    color_set = [tuple(list(i) + [255 // 9 * 8]) for i in np.random.randint(64, 256, size=[30, 3])]
    sendto_signal = QtCore.pyqtSignal(bytes)
    cmd_signal = QtCore.pyqtSignal(str)
    maxLine = 100
    infotext = []
    dic_para = {}

    socket_thread = None
    data_record = {}
    upper_plot_list = []
    bottom_plot_list = []
    addr_now = None

    name = "游客"
    # endregion

    def __init__(self, ip=None, port=None, username="default", password=12345):
        super(MyWidget, self).__init__()
        self.display_list = []

        if ip is not None and port is not None:
            self.addr_now = (str(ip), int(port))
        self.user_info = (username, password)
        self.setWindowTitle("聊天窗口")
        self.lb1 = QtWidgets.QLabel("发送到")
        self.the_output = ChatWindow(self)
        # self.the_output = QtWidgets.QTextEdit()
        # self.the_output.setReadOnly(True)

        self.the_input = QtWidgets.QTextEdit()

        self.send_select = QtWidgets.QComboBox()
        self.send_select.addItems(["所有人"])
        self.send_select.activated.connect(self.changefunc)

        # 按钮组
        self.btn1 = QtWidgets.QPushButton('发送', self)
        self.btn1.clicked.connect(self.button_Send)
        self.btn2 = QtWidgets.QPushButton('客户端信息', self)
        self.btn2.clicked.connect(self.Client_info)
        self.btn3 = QtWidgets.QPushButton('链接客户端', self)
        self.btn3.clicked.connect(self.Client_link)

        # 布局
        layout = QtWidgets.QGridLayout()
        self.setLayout(layout)

        layout.addWidget(self.the_output, 0, 0, 1, 5)
        layout.addWidget(self.the_input, 2, 0, 1, 5)
        layout.addWidget(self.lb1, 1, 0, 1, 1)
        layout.addWidget(self.send_select, 1, 1, 1, 2)
        layout.addWidget(self.btn2, 1, 4, 1, 1)
        layout.addWidget(self.btn1, 3, 4, 1, 1)
        layout.addWidget(self.btn3, 3, 3, 1, 1)

        layout.setRowStretch(0, 7)
        layout.setRowStretch(2, 1)
        layout.setRowStretch(3, 1)

    def __del__(self):
        print("del")
        if self.socket_thread is not None and self.socket_thread.isRunning():
            self.socket_thread.stop()
            print("close socket_thread")

    def connect(self, type, addr):
        if type == "UDP" or type == "TCP":
            if self.socket_thread is not None:
                self.cmd_signal.emit("stop")
            self.socket_thread = MySocketThread()
            self.socket_thread.quick_set(type, addr)
        else:
            raise Exception("ERROR: Unknown type" + type)
        self.socket_thread.send_recv_signal.connect(self.get_data)
        self.socket_thread.stop_signal.connect(lambda: None)
        self.cmd_signal.connect(self.socket_thread.signal_cmd)
        self.sendto_signal.connect(self.socket_thread.signal_sendto)
        self.socket_thread.start()

    # region //在此处处理从client<C++>收到的信息
    def get_data(self, recv_data:bytes):
        if recv_data == b"":
            self.show_warning("connect finished...(0)\n")
            self.__del__()
            return
        if recv_data == b"EOF\x00":
            self.show_warning("connect finished...(EOF)\n")
            self.__del__()
            return

        curTime = str(datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S'))
        userName = "default"
        data = '<div align="left" style="color: blue">' + userName + " " + curTime + '</div>' + \
               '<div align="left" style="font-size: 20px;font-weight: bolder">' + recv_data.decode() + '</div>'
        self.the_output.wordShow(data,"L")
    # endregion

    # region //此处因为点击发送按钮发送了信息 头部为0
    def button_Send(self):
        if self.socket_thread is not None:
            strdata = self.the_input.toPlainText()
            curTime = str(datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S'))
            userName = self.name
            data = '<div align="right" style="color: green">' + userName + " " + curTime + '</div>' + \
                   '<div align="right" style="font-size: 20px;font-weight: bolder">' + strdata + '</div>'
            # self.sendto_signal.emit(b'\0'+strdata.encode())
            self.sendto_signal.emit(make_package(strdata, 'm'))
            self.the_output.wordShow(data=data, loc="R")
        else:
            print("请先链接客户端")

        self.the_input.clear()
        return
    # endregion

    # region //通过拉起小窗口更改客户端信息，也是按钮触发的
    def Client_info(self):
        addr_lst = list(self.addr_now)
        user_info = list(self.user_info)
        old_user_info = copy.deepcopy(self.user_info)

        child_window = Child(addr_lst, user_info)
        child_window.exec()
        self.addr_now = tuple(addr_lst)
        self.user_info = tuple(user_info)
        if self.socket_thread is not None:
            if old_user_info!=self.user_info:
                if old_user_info[0]==self.user_info[0]:
                    self.sendto_signal.emit(make_package(self.user_info, 'c'))
                else:
                    self.sendto_signal.emit(make_package(self.user_info, 's'))
    # endregion

    def Client_link(self):
        # 考虑在这里调用system 拉起一个客户端  127.0.0.1 60001
        if self.addr_now is None:
            print("请完善客户端地址...")
            return
        if self.socket_thread is not None:
            self.cmd_signal.emit("stop")
        os.system(os.path.join(os.path.dirname(__file__), client_path)+" "+self.addr_now[0]+" "+str(self.addr_now[1])+" &")
        time.sleep(1.)
        self.connect("TCP", self.addr_now)
        print("client<-·-·-·->gui link success")
        self.sendto_signal.emit(make_package(self.user_info, 's'))

    def show_warning(self, text):
        print("\033[31m[*]Warning: "+text+"\033[0m")

    def changefunc(self):
        self.show_warning("<发送至>函数暂未完成")

class Child(QtWidgets.QDialog):
    def __init__(self, address: list = None, userinfo: list = None):
        super().__init__()
        if address is None:
            raise TypeError("class Child need a list pattern like [ip:str,port:int] ")
        self.addr_lst = address
        self.user_info = userinfo
        self.setWindowTitle("客户端信息")

        self.lb1 = QtWidgets.QLabel("客户端链接")
        self.lb2 = QtWidgets.QLabel("IP:")
        self.lb3 = QtWidgets.QLabel("Port:")
        self.ip_info = QtWidgets.QLineEdit()
        self.port_info = QtWidgets.QLineEdit()
        self.ip_info.setText(str(self.addr_lst[0]))
        self.port_info.setText(str(self.addr_lst[1]))

        self.lb4 = QtWidgets.QLabel("用户信息")
        self.lb5 = QtWidgets.QLabel("用户名:")
        self.lb6 = QtWidgets.QLabel("密码:")
        self.username_info = QtWidgets.QLineEdit()
        self.password_info = QtWidgets.QLineEdit()
        self.password_info.setEchoMode(QtWidgets.QLineEdit.Password)
        self.username_info.setText(str(self.user_info[0]))
        self.password_info.setText(str(self.user_info[1]))

        self.btn1 = QtWidgets.QPushButton('OK', self)
        self.btn1.clicked.connect(lambda: self.accept(True))
        self.btn2 = QtWidgets.QPushButton('Apply', self)
        self.btn2.clicked.connect(lambda: self.accept())
        self.btn3 = QtWidgets.QPushButton('Cancel', self)
        self.btn3.clicked.connect(self.close)

        layout = QtWidgets.QGridLayout()
        self.setLayout(layout)
        layout.addWidget(self.lb1, 0, 0, 1, 5)
        layout.addWidget(self.lb2, 1, 0, 1, 1)
        layout.addWidget(self.ip_info, 1, 1, 1, 4)

        layout.addWidget(self.lb3, 2, 0, 1, 1)
        layout.addWidget(self.port_info, 2, 1, 1, 4)

        layout.addWidget(self.lb4, 3, 0, 1, 5)
        layout.addWidget(self.lb5, 4, 0, 1, 1)
        layout.addWidget(self.username_info, 4, 1, 1, 4)

        layout.addWidget(self.lb6, 5, 0, 1, 1)
        layout.addWidget(self.password_info, 5, 1, 1, 4)

        layout.addWidget(self.btn1, 6, 2, 1, 1)
        layout.addWidget(self.btn2, 6, 1, 1, 1)
        layout.addWidget(self.btn3, 6, 0, 1, 1)

    def accept(self, exit_=False):
        try:
            self.addr_lst[0] = self.ip_info.text()
            self.addr_lst[1] = int(self.port_info.text())
            self.user_info[0] = self.username_info.text()
            self.user_info[1] = int(self.password_info.text())
        except Exception as z:
            print("\033[31mERROR: "+str(z)+"\033[0m")
        if exit_:
            self.close()




if __name__ == '__main__':
    if len(sys.argv) != 4:
        print(f"[-] Usage1: {sys.argv[0]} <TCP> <IP_Server_addr> <WindowName>\n"
              f"[-] Usage2: {sys.argv[0]} <UDP> <IP_Server_addr> <WindowName>\n"
              f"[*] Example:client.py UDP 192.168.11.5:6000 Console-31\n")
        print(sys.argv)
        print("Now we use the default parameter: UDP 127.0.0.1:<random int(10000~60000)> default-name")
        type_, server_addr, name = "UDP", "127.0.0.1:"+str(random.randint(10000, 60000)), "default-name"
    else:
        type_ = sys.argv[1]
        server_addr = sys.argv[2]
        name = sys.argv[3]

    raw_str = re.split(":", server_addr)
    assert len(raw_str) == 2
    addr = tuple([raw_str[0], int(raw_str[1])])

    app = QtWidgets.QApplication([])
    w = MyWidget(ip=addr[0], port=addr[1])
    w.show()
    app.exec_()
    exit()
