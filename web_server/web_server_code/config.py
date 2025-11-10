import os
from pathlib import Path

# 基础路径配置
BASE_DIR = Path(__file__).resolve().parent.parent  # 项目根目录
WEB_DIR = Path(__file__).resolve().parent          # web服务器目录

# 日志目录配置
CLIENT_LOGS_DIR = os.path.join(BASE_DIR, 'client_logs')
SERVER_LOGS_DIR = os.path.join(BASE_DIR, 'server_logs')

# 服务器配置
HOST = '0.0.0.0'  # 监听所有网络接口
PORT = 5000
DEBUG = True

# 日志文件配置
LOG_ENCODING = 'utf-8'
MAX_LOG_LINES = 1000  # 默认最多读取的日志行数

# 文件大小限制（字节）
MAX_FILE_SIZE = 100 * 1024 * 1024  # 100MB

# 允许的文件扩展名
ALLOWED_EXTENSIONS = {'.log'}

# 分页配置
DEFAULT_PAGE_SIZE = 100
MAX_PAGE_SIZE = 1000

# CORS 配置
CORS_ORIGINS = '*'  # 生产环境建议设置具体域名

# 缓存配置
ENABLE_CACHE = False
CACHE_TIMEOUT = 300  # 5分钟

print(f"配置加载完成:")
print(f"  基础目录: {BASE_DIR}")
print(f"  Web目录: {WEB_DIR}")
print(f"  客户端日志: {CLIENT_LOGS_DIR}")
print(f"  服务端日志: {SERVER_LOGS_DIR}")