import os

# 基础配置
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
CLIENT_LOGS_DIR = os.path.join(BASE_DIR, 'client_logs')
SERVER_LOGS_DIR = os.path.join(BASE_DIR, 'server_logs')

# Flask配置
DEBUG = True
HOST = '0.0.0.0'
PORT = 5000

# 日志配置
LOG_ENCODING = 'utf-8'
MAX_LOG_LINES = 10000
