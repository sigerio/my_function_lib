from flask import Flask, jsonify, request, send_from_directory
from flask_cors import CORS
import os
import json
from datetime import datetime
import config

app = Flask(__name__)
CORS(app)

# IEC104 帧类型定义
IEC104_FRAME_TYPES = {
    'I': 'I帧(信息传输)',
    'S': 'S帧(监视)',
    'U': 'U帧(控制)',
}

# IEC104 U帧功能
U_FRAME_FUNCTIONS = {
    0x07: 'STARTDT act',
    0x0B: 'STARTDT con',
    0x13: 'STOPDT act',
    0x23: 'STOPDT con',
    0x43: 'TESTFR act',
    0x83: 'TESTFR con'
}

# IEC104 类型标识
TYPE_IDENTIFICATION = {
    0x01: 'M_SP_NA_1 单点信息',
    0x03: 'M_DP_NA_1 双点信息',
    0x09: 'M_ME_NA_1 测量值,归一化值',
    0x0B: 'M_ME_TA_1 测量值,标度化值,带时标',
    0x0D: 'M_ME_NC_1 测量值,短浮点数',
    0x0F: 'M_IT_NA_1 累计量',
    0x1E: 'M_SP_TB_1 带时标的单点信息',
    0x1F: 'M_DP_TB_1 带时标的双点信息',
    0x24: 'M_ME_TF_1 带时标的测量值,短浮点数',
    0x2D: 'C_SC_NA_1 单命令',
    0x2E: 'C_DC_NA_1 双命令',
    0x2F: 'C_RC_NA_1 调节命令',
    0x30: 'C_SE_NA_1 设定值命令,归一化值',
    0x31: 'C_SE_NB_1 设定值命令,标度化值',
    0x32: 'C_SE_NC_1 设定值命令,短浮点数',
    0x64: 'C_IC_NA_1 总召唤命令',
    0x65: 'C_CI_NA_1 电能脉冲召唤命令',
    0x67: 'C_CS_NA_1 时钟同步命令',
    0x68: 'C_TS_NA_1 测试命令',
    0x69: 'C_RP_NA_1 复位进程命令',
    0x6A: 'C_CD_NA_1 延时获得命令'
}

def parse_iec104_frame(data_str):
    """解析IEC104帧"""
    try:
        # 将十六进制字符串转换为字节数组
        bytes_data = [int(x, 16) for x in data_str.strip().split()]
        
        if len(bytes_data) < 6:
            return {'type': 'INVALID', 'description': '帧长度不足'}
        
        # 检查启动字符
        if bytes_data[0] != 0x68:
            return {'type': 'INVALID', 'description': '无效的启动字符'}
        
        # 获取APDU长度
        apdu_len = bytes_data[1]
        
        # 获取控制域
        ctrl1 = bytes_data[2]
        ctrl2 = bytes_data[3]
        ctrl3 = bytes_data[4]
        ctrl4 = bytes_data[5]
        
        frame_info = {
            'apdu_len': apdu_len,
            'ctrl': f'{ctrl1:02X} {ctrl2:02X} {ctrl3:02X} {ctrl4:02X}'
        }
        
        # 判断帧类型
        if (ctrl1 & 0x01) == 0:  # I帧
            frame_info['type'] = 'I'
            frame_info['type_desc'] = IEC104_FRAME_TYPES['I']
            frame_info['send_seq'] = ((ctrl1 & 0xFE) >> 1) | ((ctrl2 & 0xFF) << 7)
            frame_info['recv_seq'] = ((ctrl3 & 0xFE) >> 1) | ((ctrl4 & 0xFF) << 7)
            
            # 解析ASDU
            if len(bytes_data) >= 12:
                type_id = bytes_data[6]
                frame_info['type_id'] = f'0x{type_id:02X}'
                frame_info['type_id_desc'] = TYPE_IDENTIFICATION.get(type_id, '未知类型')
                frame_info['sq'] = (bytes_data[7] & 0x80) >> 7
                frame_info['num_obj'] = bytes_data[7] & 0x7F
                frame_info['cause'] = bytes_data[8]
                frame_info['asdu_addr'] = bytes_data[10] | (bytes_data[11] << 8)
                
                # 原因传输
                cause_desc = {
                    3: '突发',
                    5: '被请求',
                    6: '激活',
                    7: '激活确认',
                    8: '停止激活',
                    9: '停止激活确认',
                    10: '激活终止',
                    20: '响应总召唤',
                    37: '响应电能召唤'
                }.get(frame_info['cause'], '未知原因')
                frame_info['cause_desc'] = cause_desc
                
        elif (ctrl1 & 0x03) == 0x01:  # S帧
            frame_info['type'] = 'S'
            frame_info['type_desc'] = IEC104_FRAME_TYPES['S']
            frame_info['recv_seq'] = ((ctrl3 & 0xFE) >> 1) | ((ctrl4 & 0xFF) << 7)
            
        elif (ctrl1 & 0x03) == 0x03:  # U帧
            frame_info['type'] = 'U'
            frame_info['type_desc'] = IEC104_FRAME_TYPES['U']
            frame_info['function'] = U_FRAME_FUNCTIONS.get(ctrl1, f'未知功能(0x{ctrl1:02X})')
        
        return frame_info
        
    except Exception as e:
        return {'type': 'ERROR', 'description': f'解析错误: {str(e)}'}

def parse_log_line_json(line):
    """解析JSON格式的日志行"""
    try:
        log_data = json.loads(line)
        
        # 转换时间戳
        timestamp_ms = log_data.get('time_ms', 0)
        timestamp = datetime.fromtimestamp(timestamp_ms / 1000)
        timestamp_str = timestamp.strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
        
        # 解析方向
        direction = log_data.get('dir', '')
        if 'ser -> cli' in direction:
            dir_type = 'TX'
            dir_desc = '服务端→客户端'
        elif 'cli -> ser' in direction:
            dir_type = 'RX'
            dir_desc = '客户端→服务端'
        else:
            dir_type = 'UNKNOWN'
            dir_desc = direction
        
        # 解析IEC104帧
        data_hex = log_data.get('data', '')
        frame_info = parse_iec104_frame(data_hex) if data_hex else {}
        
        return {
            'timestamp': timestamp_str,
            'timestamp_ms': timestamp_ms,
            'direction': dir_type,
            'direction_desc': dir_desc,
            'length': log_data.get('len', 0),
            'data': data_hex,
            'frame_info': frame_info,
            'raw': line
        }
    except json.JSONDecodeError:
        return None
    except Exception as e:
        print(f"解析日志行失败: {e}")
        return None

def parse_iec104_log_file(filepath, tail_lines=None):
    """解析IEC104 JSON格式日志文件"""
    logs = []
    
    if not os.path.exists(filepath):
        return logs
    
    try:
        with open(filepath, 'r', encoding=config.LOG_ENCODING, errors='ignore') as f:
            lines = f.readlines()
            
            if tail_lines:
                lines = lines[-tail_lines:]
            else:
                lines = lines[-config.MAX_LOG_LINES:]
            
            for line_num, line in enumerate(lines, 1):
                line = line.strip()
                if not line:
                    continue
                
                log_entry = parse_log_line_json(line)
                if log_entry:
                    log_entry['line_num'] = line_num
                    logs.append(log_entry)
    
    except Exception as e:
        print(f"读取日志文件失败 {filepath}: {e}")
    
    return logs

def get_log_files():
    """获取所有日志文件列表"""
    files = {
        'client': [],
        'server': []
    }
    
    # 获取客户端日志文件
    if os.path.exists(config.CLIENT_LOGS_DIR):
        for filename in os.listdir(config.CLIENT_LOGS_DIR):
            if filename.endswith('.log'):
                filepath = os.path.join(config.CLIENT_LOGS_DIR, filename)
                stats = os.stat(filepath)
                files['client'].append({
                    'name': filename,
                    'path': filepath,
                    'size': stats.st_size,
                    'modified': datetime.fromtimestamp(stats.st_mtime).strftime('%Y-%m-%d %H:%M:%S')
                })
    
    # 获取服务端日志文件
    if os.path.exists(config.SERVER_LOGS_DIR):
        for filename in os.listdir(config.SERVER_LOGS_DIR):
            if filename.endswith('.log'):
                filepath = os.path.join(config.SERVER_LOGS_DIR, filename)
                stats = os.stat(filepath)
                files['server'].append({
                    'name': filename,
                    'path': filepath,
                    'size': stats.st_size,
                    'modified': datetime.fromtimestamp(stats.st_mtime).strftime('%Y-%m-%d %H:%M:%S')
                })
    
    return files

@app.route('/')
def index():
    return send_from_directory('static', 'index.html')

@app.route('/static/<path:path>')
def send_static(path):
    return send_from_directory('static', path)

@app.route('/api/files', methods=['GET'])
def get_files():
    try:
        files = get_log_files()
        return jsonify(files)
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/logs', methods=['GET'])
def get_logs():
    try:
        filepath = request.args.get('file')
        log_type = request.args.get('type', 'client')
        tail_lines = request.args.get('tail', type=int)
        
        if not filepath:
            return jsonify({'error': '未指定文件'}), 400
        
        base_dir = config.CLIENT_LOGS_DIR if log_type == 'client' else config.SERVER_LOGS_DIR
        full_path = os.path.join(base_dir, os.path.basename(filepath))
        
        if not os.path.exists(full_path):
            return jsonify({'error': '文件不存在'}), 404
        
        logs = parse_iec104_log_file(full_path, tail_lines)
        
        file_info = {
            'name': os.path.basename(filepath),
            'type': log_type,
            'total_lines': len(logs),
            'file_size': os.path.getsize(full_path)
        }
        
        return jsonify({
            'file_info': file_info,
            'logs': logs
        })
    
    except Exception as e:
        return jsonify({'error': str(e)}), 500

if __name__ == '__main__':
    os.makedirs(config.CLIENT_LOGS_DIR, exist_ok=True)
    os.makedirs(config.SERVER_LOGS_DIR, exist_ok=True)
    os.makedirs('static', exist_ok=True)
    
    app.run(debug=config.DEBUG, host=config.HOST, port=config.PORT)