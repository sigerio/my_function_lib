from flask import Flask, jsonify, request, send_from_directory
from flask_cors import CORS
import os
import json
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Optional, Any
import config

app = Flask(__name__)
CORS(app, origins=config.CORS_ORIGINS)

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
    0x0B: 'M_ME_TB_1 测量值,标度化值,带时标',
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

# 传输原因
CAUSE_OF_TRANSMISSION = {
    1: '周期循环',
    2: '背景扫描',
    3: '突发',
    4: '初始化',
    5: '被请求',
    6: '激活',
    7: '激活确认',
    8: '停止激活',
    9: '停止激活确认',
    10: '激活终止',
    11: '远程命令引起的返送信息',
    12: '当地命令引起的返送信息',
    13: '文件传输',
    20: '响应总召唤',
    21: '响应第1组召唤',
    36: '响应计数量总召唤',
    37: '响应第1组计数量召唤',
    44: '未知类型标识',
    45: '未知传送原因',
    46: '未知公共地址',
    47: '未知信息对象地址'
}

class IEC104FrameParser:
    """IEC104帧解析器"""
    
    APCI_START = 0x68
    MIN_FRAME_LENGTH = 6
    ASDU_MIN_LENGTH = 12
    
    @staticmethod
    def parse_hex_string(data_str: str) -> Optional[List[int]]:
        """解析十六进制字符串为字节数组"""
        try:
            # 支持多种格式并去除首尾空格
            data_str = data_str.strip()  # 添加这行
            # 过滤空字符串
            parts = [x for x in data_str.split() if x]  # 修改这行
            return [int(x, 16) for x in parts]
        except (ValueError, IndexError) as e:
            print(f"十六进制解析错误: {e}")
            return None
    
    @staticmethod
    def parse_i_frame(bytes_data: List[int], frame_info: Dict[str, Any]) -> None:
        """解析I帧"""
        ctrl1, ctrl2, ctrl3, ctrl4 = bytes_data[2:6]
        
        frame_info['type'] = 'I'
        frame_info['type_desc'] = IEC104_FRAME_TYPES['I']
        frame_info['send_seq'] = ((ctrl1 & 0xFE) >> 1) | ((ctrl2 & 0xFF) << 7)
        frame_info['recv_seq'] = ((ctrl3 & 0xFE) >> 1) | ((ctrl4 & 0xFF) << 7)
        
        # 解析ASDU
        if len(bytes_data) >= IEC104FrameParser.ASDU_MIN_LENGTH:
            IEC104FrameParser.parse_asdu(bytes_data, frame_info)
    
    @staticmethod
    def parse_s_frame(bytes_data: List[int], frame_info: Dict[str, Any]) -> None:
        """解析S帧"""
        ctrl3, ctrl4 = bytes_data[4:6]
        
        frame_info['type'] = 'S'
        frame_info['type_desc'] = IEC104_FRAME_TYPES['S']
        frame_info['recv_seq'] = ((ctrl3 & 0xFE) >> 1) | ((ctrl4 & 0xFF) << 7)
    
    @staticmethod
    def parse_u_frame(bytes_data: List[int], frame_info: Dict[str, Any]) -> None:
        """解析U帧"""
        ctrl1 = bytes_data[2]
        
        frame_info['type'] = 'U'
        frame_info['type_desc'] = IEC104_FRAME_TYPES['U']
        frame_info['function'] = U_FRAME_FUNCTIONS.get(ctrl1, f'未知功能(0x{ctrl1:02X})')
    
    @staticmethod
    def parse_asdu(bytes_data: List[int], frame_info: Dict[str, Any]) -> None:
        """解析ASDU（应用服务数据单元）"""
        type_id = bytes_data[6]
        vsq = bytes_data[7]
        cause = bytes_data[8]
        
        frame_info['type_id'] = f'0x{type_id:02X}'
        frame_info['type_id_desc'] = TYPE_IDENTIFICATION.get(type_id, '未知类型')
        frame_info['sq'] = (vsq & 0x80) >> 7  # 顺序标志
        frame_info['num_obj'] = vsq & 0x7F   # 信息对象数量
        frame_info['cause'] = cause & 0x3F    # 传输原因（低6位）
        frame_info['cause_desc'] = CAUSE_OF_TRANSMISSION.get(
            cause & 0x3F, 
            f'未知原因({cause & 0x3F})'
        )
        frame_info['test'] = (cause & 0x80) >> 7  # 测试标志
        frame_info['pn'] = (cause & 0x40) >> 6     # 肯定/否定标志
        
        # ASDU地址（2字节）
        if len(bytes_data) >= 12:
            frame_info['asdu_addr'] = bytes_data[10] | (bytes_data[11] << 8)
        
        # 信息对象地址（如果有）
        if len(bytes_data) >= 15:
            frame_info['ioa'] = (bytes_data[12] | 
                               (bytes_data[13] << 8) | 
                               (bytes_data[14] << 16))
            
        # 尝试解析信息元素
        if len(bytes_data) >= 16:
            IEC104FrameParser.parse_information_elements(bytes_data, frame_info, type_id)
    
    @staticmethod
    def parse_information_elements(bytes_data: List[int], frame_info: Dict[str, Any], type_id: int) -> None:
        """解析信息元素"""
        try:
            if type_id == 0x64:  # 总召唤
                if len(bytes_data) >= 16:
                    qoi = bytes_data[15]
                    frame_info['qoi'] = qoi
                    frame_info['qoi_desc'] = '总召唤' if qoi == 20 else f'召唤{qoi}'
            
            elif type_id in [0x01, 0x1E]:  # 单点信息
                if len(bytes_data) >= 16:
                    siq = bytes_data[15]
                    frame_info['value'] = siq & 0x01
                    frame_info['quality'] = {
                        'blocked': (siq & 0x10) >> 4,
                        'substituted': (siq & 0x20) >> 5,
                        'not_topical': (siq & 0x40) >> 6,
                        'invalid': (siq & 0x80) >> 7
                    }
            
            elif type_id in [0x0D, 0x24]:  # 短浮点数
                if len(bytes_data) >= 19:
                    import struct
                    value_bytes = bytes(bytes_data[15:19])
                    value = struct.unpack('<f', value_bytes)[0]
                    frame_info['value'] = round(value, 4)
                    if len(bytes_data) >= 20:
                        qds = bytes_data[19]
                        frame_info['quality'] = {
                            'overflow': qds & 0x01,
                            'blocked': (qds & 0x10) >> 4,
                            'substituted': (qds & 0x20) >> 5,
                            'not_topical': (qds & 0x40) >> 6,
                            'invalid': (qds & 0x80) >> 7
                        }
        except Exception as e:
            print(f"解析信息元素失败: {e}")
    
    @classmethod
    def parse(cls, data_str: str) -> Dict[str, Any]:
        """解析IEC104帧"""
        if not data_str or not data_str.strip():
            return {'type': 'INVALID', 'description': '空数据'}
        
        bytes_data = cls.parse_hex_string(data_str)
        if bytes_data is None:
            return {'type': 'INVALID', 'description': '十六进制格式错误'}
        
        if len(bytes_data) < cls.MIN_FRAME_LENGTH:
            return {
                'type': 'INVALID', 
                'description': f'帧长度不足（需要至少{cls.MIN_FRAME_LENGTH}字节，实际{len(bytes_data)}字节）'
            }
        
        # 检查启动字符
        if bytes_data[0] != cls.APCI_START:
            return {
                'type': 'INVALID', 
                'description': f'无效的启动字符: 0x{bytes_data[0]:02X} (期望 0x68)'
            }
        
        # 获取APDU长度
        apdu_len = bytes_data[1]
        
        # 验证实际长度
        expected_len = apdu_len + 2  # APDU长度 + 启动字符 + 长度字节
        if len(bytes_data) < expected_len:
            print(f"警告: 实际长度({len(bytes_data)}) < 期望长度({expected_len})")
        
        ctrl1 = bytes_data[2]
        
        frame_info = {
            'apdu_len': apdu_len,
            'ctrl': ' '.join(f'{b:02X}' for b in bytes_data[2:6] if len(bytes_data) > 5),
            'raw_bytes': ' '.join(f'{b:02X}' for b in bytes_data),
            'byte_count': len(bytes_data)
        }
        
        try:
            # 判断帧类型
            if (ctrl1 & 0x01) == 0:  # I帧
                cls.parse_i_frame(bytes_data, frame_info)
            elif (ctrl1 & 0x03) == 0x01:  # S帧
                cls.parse_s_frame(bytes_data, frame_info)
            elif (ctrl1 & 0x03) == 0x03:  # U帧
                cls.parse_u_frame(bytes_data, frame_info)
            else:
                frame_info['type'] = 'UNKNOWN'
                frame_info['description'] = f'未知帧类型: 0x{ctrl1:02X}'
        
        except Exception as e:
            return {'type': 'ERROR', 'description': f'解析错误: {str(e)}'}
        
        return frame_info


def parse_log_line_json(line: str) -> Optional[Dict[str, Any]]:
    """解析JSON格式的日志行"""
    try:
        # 去除首尾空白字符
        line = line.strip()
        if not line:
            return None
            
        log_data = json.loads(line)
        
        # 转换时间戳
        timestamp_ms = log_data.get('time_ms', 0)
        if timestamp_ms > 0:
            timestamp = datetime.fromtimestamp(timestamp_ms / 1000)
            timestamp_str = timestamp.strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
        else:
            timestamp_str = 'N/A'
        
        # 解析方向
        direction = log_data.get('dir', '')
        if 'ser -> cli' in direction or 'server' in direction.lower():
            dir_type = 'TX'
            dir_desc = '服务端→客户端'
        elif 'cli -> ser' in direction or 'client' in direction.lower():
            dir_type = 'RX'
            dir_desc = '客户端→服务端'
        else:
            dir_type = 'UNKNOWN'
            dir_desc = direction or '未知方向'
        
        # 解析IEC104帧
        data_hex = log_data.get('data', '').strip()
        frame_info = IEC104FrameParser.parse(data_hex) if data_hex else {}
        
        return {
            'timestamp': timestamp_str,
            'timestamp_ms': timestamp_ms,
            'direction': dir_type,
            'direction_desc': dir_desc,
            'length': log_data.get('len', len(data_hex.split()) if data_hex else 0),
            'data': data_hex,
            'frame_info': frame_info,
            'raw': line
        }
    
    except json.JSONDecodeError as e:
        # 添加更详细的错误信息
        print(f"JSON解析错误: {e}")
        print(f"错误位置: 第{e.lineno}行, 第{e.colno}列")
        print(f"问题行内容: {line[:100]}...")  # 只显示前100个字符
        return None
    except Exception as e:
        print(f"解析日志行失败: {e}, 行内容: {line[:100]}")
        return None

def parse_iec104_log_file(filepath: str, tail_lines: Optional[int] = None, 
                          filter_type: Optional[str] = None) -> List[Dict[str, Any]]:
    """解析IEC104 JSON格式日志文件"""
    logs = []
    
    if not os.path.exists(filepath):
        print(f"文件不存在: {filepath}")
        return logs
    
    # 检查文件大小
    file_size = os.path.getsize(filepath)
    if file_size > config.MAX_FILE_SIZE:
        print(f"警告: 文件过大 ({format_file_size(file_size)}), 可能影响性能")
    
    try:
        with open(filepath, 'r', encoding=config.LOG_ENCODING, errors='ignore') as f:
            lines = f.readlines()
            
            # 确定要读取的行数
            if tail_lines and tail_lines > 0:
                lines = lines[-min(tail_lines, len(lines)):]
            else:
                max_lines = config.MAX_LOG_LINES
                lines = lines[-min(max_lines, len(lines)):]
            
            # 添加原始行号跟踪
            for original_line_num, (line_num, line) in enumerate(enumerate(lines, 1), 1):
                line = line.strip()
                if not line:
                    continue
                
                log_entry = parse_log_line_json(line)
                if log_entry:
                    # 应用过滤器
                    if filter_type:
                        frame_type = log_entry.get('frame_info', {}).get('type', '')
                        if frame_type != filter_type:
                            continue
                    
                    log_entry['line_num'] = line_num
                    log_entry['file_line_num'] = original_line_num  # 添加文件中的实际行号
                    logs.append(log_entry)
                else:
                    # 记录解析失败的行号
                    print(f"行 {original_line_num} 解析失败")
    
    except Exception as e:
        print(f"读取日志文件失败 {filepath}: {e}")
    
    return logs

def get_log_files() -> Dict[str, List[Dict[str, Any]]]:
    """获取所有日志文件列表"""
    files = {
        'client': [],
        'server': []
    }
    
    def scan_directory(directory: str, file_type: str) -> List[Dict[str, Any]]:
        """扫描目录获取日志文件"""
        result = []
        if not os.path.exists(directory):
            print(f"目录不存在: {directory}")
            os.makedirs(directory, exist_ok=True)
            return result
        
        try:
            for filename in os.listdir(directory):
                if filename.endswith('.log'):
                    filepath = os.path.join(directory, filename)
                    try:
                        stats = os.stat(filepath)
                        result.append({
                            'name': filename,
                            'path': filepath,
                            'size': stats.st_size,
                            'size_formatted': format_file_size(stats.st_size),
                            'modified': datetime.fromtimestamp(stats.st_mtime).strftime('%Y-%m-%d %H:%M:%S'),
                            'modified_ts': stats.st_mtime,
                            'type': file_type
                        })
                    except OSError as e:
                        print(f"获取文件信息失败 {filepath}: {e}")
        except OSError as e:
            print(f"扫描目录失败 {directory}: {e}")
        
        return sorted(result, key=lambda x: x['modified_ts'], reverse=True)
    
    # 获取客户端和服务端日志文件
    files['client'] = scan_directory(config.CLIENT_LOGS_DIR, 'client')
    files['server'] = scan_directory(config.SERVER_LOGS_DIR, 'server')
    
    return files


def format_file_size(size_bytes: int) -> str:
    """格式化文件大小"""
    for unit in ['B', 'KB', 'MB', 'GB']:
        if size_bytes < 1024.0:
            return f"{size_bytes:.2f} {unit}"
        size_bytes /= 1024.0
    return f"{size_bytes:.2f} TB"


def validate_file_path(filepath: str, base_dir: str) -> Optional[str]:
    """验证文件路径安全性"""
    try:
        # 获取规范化的绝对路径
        base_path = Path(base_dir).resolve()
        file_path = (base_path / os.path.basename(filepath)).resolve()
        
        # 检查路径是否在基础目录内
        if base_path in file_path.parents or base_path == file_path.parent:
            if file_path.exists() and file_path.is_file():
                return str(file_path)
        
        return None
    except Exception as e:
        print(f"路径验证失败: {e}")
        return None


# ==================== API 路由 ====================

@app.route('/')
def index():
    """首页"""
    static_dir = os.path.join(os.path.dirname(__file__), 'static')
    return send_from_directory(static_dir, 'index.html')


@app.route('/static/<path:path>')
def send_static(path):
    """静态文件"""
    static_dir = os.path.join(os.path.dirname(__file__), 'static')
    return send_from_directory(static_dir, path)


@app.route('/api/config', methods=['GET'])
def get_config():
    """获取配置信息"""
    return jsonify({
        'success': True,
        'config': {
            'max_log_lines': config.MAX_LOG_LINES,
            'max_file_size': config.MAX_FILE_SIZE,
            'max_file_size_formatted': format_file_size(config.MAX_FILE_SIZE),
            'client_logs_dir': config.CLIENT_LOGS_DIR,
            'server_logs_dir': config.SERVER_LOGS_DIR
        }
    })


@app.route('/api/files', methods=['GET'])
def get_files():
    """获取文件列表"""
    try:
        files = get_log_files()
        return jsonify({
            'success': True,
            'data': files,
            'total': {
                'client': len(files['client']),
                'server': len(files['server'])
            }
        })
    except Exception as e:
        return jsonify({
            'success': False,
            'error': str(e)
        }), 500


@app.route('/api/logs', methods=['GET'])
def get_logs():
    """获取日志内容"""
    try:
        filepath = request.args.get('file')
        log_type = request.args.get('type', 'client')
        tail_lines = request.args.get('tail', type=int)
        filter_type = request.args.get('filter')  # I, S, U
        
        if not filepath:
            return jsonify({
                'success': False,
                'error': '未指定文件'
            }), 400
        
        # 验证日志类型
        if log_type not in ['client', 'server']:
            return jsonify({
                'success': False,
                'error': '无效的日志类型'
            }), 400
        
        # 获取基础目录
        base_dir = config.CLIENT_LOGS_DIR if log_type == 'client' else config.SERVER_LOGS_DIR
        
        # 验证文件路径
        full_path = validate_file_path(filepath, base_dir)
        if not full_path:
            return jsonify({
                'success': False,
                'error': f'文件不存在或路径无效: {filepath}'
            }), 404
        
        # 解析日志
        logs = parse_iec104_log_file(full_path, tail_lines, filter_type)
        
        # 文件信息
        file_stats = os.stat(full_path)
        file_info = {
            'name': os.path.basename(filepath),
            'type': log_type,
            'total_lines': len(logs),
            'file_size': file_stats.st_size,
            'file_size_formatted': format_file_size(file_stats.st_size),
            'modified': datetime.fromtimestamp(file_stats.st_mtime).strftime('%Y-%m-%d %H:%M:%S')
        }
        
        return jsonify({
            'success': True,
            'file_info': file_info,
            'logs': logs
        })
    
    except Exception as e:
        import traceback
        traceback.print_exc()
        return jsonify({
            'success': False,
            'error': str(e)
        }), 500


@app.route('/api/stats', methods=['GET'])
def get_stats():
    """获取统计信息"""
    try:
        filepath = request.args.get('file')
        log_type = request.args.get('type', 'client')
        
        if not filepath:
            return jsonify({
                'success': False,
                'error': '未指定文件'
            }), 400
        
        base_dir = config.CLIENT_LOGS_DIR if log_type == 'client' else config.SERVER_LOGS_DIR
        full_path = validate_file_path(filepath, base_dir)
        
        if not full_path:
            return jsonify({
                'success': False,
                'error': '文件不存在'
            }), 404
        
        logs = parse_iec104_log_file(full_path)
        
        # 统计信息
        stats = {
            'total': len(logs),
            'frame_types': {},
            'type_ids': {},
            'causes': {},
            'directions': {'TX': 0, 'RX': 0, 'UNKNOWN': 0}
        }
        
        for log in logs:
            # 方向统计
            direction = log.get('direction', 'UNKNOWN')
            stats['directions'][direction] = stats['directions'].get(direction, 0) + 1
            
            frame_info = log.get('frame_info', {})
            
            # 帧类型统计
            frame_type = frame_info.get('type', 'UNKNOWN')
            stats['frame_types'][frame_type] = stats['frame_types'].get(frame_type, 0) + 1
            
            # 类型标识统计 (仅I帧)
            if frame_type == 'I' and 'type_id_desc' in frame_info:
                type_id = frame_info['type_id_desc']
                stats['type_ids'][type_id] = stats['type_ids'].get(type_id, 0) + 1
            
            # 传输原因统计 (仅I帧)
            if frame_type == 'I' and 'cause_desc' in frame_info:
                cause = frame_info['cause_desc']
                stats['causes'][cause] = stats['causes'].get(cause, 0) + 1
        
        return jsonify({
            'success': True,
            'stats': stats
        })
    
    except Exception as e:
        return jsonify({
            'success': False,
            'error': str(e)
        }), 500


@app.errorhandler(404)
def not_found(error):
    """404错误处理"""
    return jsonify({
        'success': False,
        'error': '资源不存在'
    }), 404


@app.errorhandler(500)
def internal_error(error):
    """500错误处理"""
    return jsonify({
        'success': False,
        'error': '服务器内部错误'
    }), 500


@app.route('/api/logs/tail', methods=['GET'])
def get_logs_tail():
    """获取日志尾部（用于实时更新）"""
    try:
        filepath = request.args.get('file')
        log_type = request.args.get('type', 'client')
        since_ms = request.args.get('since', type=int, default=0)  # 获取此时间戳之后的日志
        
        if not filepath:
            return jsonify({'success': False, 'error': '未指定文件'}), 400
        
        base_dir = config.CLIENT_LOGS_DIR if log_type == 'client' else config.SERVER_LOGS_DIR
        full_path = validate_file_path(filepath, base_dir)
        
        if not full_path:
            return jsonify({'success': False, 'error': '文件不存在'}), 404
        
        # 读取所有日志
        logs = parse_iec104_log_file(full_path)
        
        # 只返回指定时间戳之后的日志
        new_logs = [log for log in logs if log.get('timestamp_ms', 0) > since_ms]
        
        return jsonify({
            'success': True,
            'logs': new_logs,
            'count': len(new_logs)
        })
    
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)}), 500

if __name__ == '__main__':
    # 确保目录存在
    os.makedirs(config.CLIENT_LOGS_DIR, exist_ok=True)
    os.makedirs(config.SERVER_LOGS_DIR, exist_ok=True)
    
    static_dir = os.path.join(os.path.dirname(__file__), 'static')
    os.makedirs(static_dir, exist_ok=True)
    
    # 启动应用
    print("\n" + "="*60)
    print("IEC104 日志查看器启动中...")
    print("="*60)
    print(f"客户端日志目录: {config.CLIENT_LOGS_DIR}")
    print(f"服务端日志目录: {config.SERVER_LOGS_DIR}")
    print(f"访问地址: http://{config.HOST}:{config.PORT}")
    print("="*60 + "\n")
    
    app.run(
        debug=config.DEBUG, 
        host=config.HOST, 
        port=config.PORT,
        threaded=True
    )