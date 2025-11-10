let currentFile = null;
let currentType = null;
let allLogs = [];
let filteredLogs = [];
let autoRefreshInterval = null;

// 页面初始化
document.addEventListener('DOMContentLoaded', function() {
    loadFileList();
    updateCurrentTime();
    setInterval(updateCurrentTime, 1000);
});

// 更新当前时间
function updateCurrentTime() {
    const now = new Date();
    document.getElementById('currentTime').textContent = 
        now.toLocaleString('zh-CN', { 
            year: 'numeric', 
            month: '2-digit', 
            day: '2-digit', 
            hour: '2-digit', 
            minute: '2-digit', 
            second: '2-digit' 
        });
}

// 加载文件列表
async function loadFileList() {
    try {
        const response = await fetch('/api/files');
        const data = await response.json();
        
        displayFileList('clientFiles', data.client, 'client');
        displayFileList('serverFiles', data.server, 'server');
    } catch (error) {
        console.error('加载文件列表失败:', error);
    }
}

// 显示文件列表
function displayFileList(containerId, files, type) {
    const container = document.getElementById(containerId);
    container.innerHTML = '';
    
    if (files.length === 0) {
        container.innerHTML = '<div class="no-files">暂无日志文件</div>';
        return;
    }
    
    files.forEach(file => {
        const fileItem = document.createElement('div');
        fileItem.className = 'file-item';
        fileItem.onclick = () => selectFile(file.name, type);
        
        fileItem.innerHTML = `
            <div class="file-name">${file.name}</div>
            <div class="file-meta">
                <span>${formatFileSize(file.size)}</span>
                <span>${file.modified}</span>
            </div>
        `;
        
        container.appendChild(fileItem);
    });
}

// 格式化文件大小
function formatFileSize(bytes) {
    if (bytes === 0) return '0 B';
    const k = 1024;
    const sizes = ['B', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return Math.round(bytes / Math.pow(k, i) * 100) / 100 + ' ' + sizes[i];
}

// 选择文件
function selectFile(filename, type) {
    document.querySelectorAll('.file-item').forEach(item => {
        item.classList.remove('active');
        if (item.querySelector('.file-name').textContent === filename) {
            item.classList.add('active');
        }
    });
    
    currentFile = filename;
    currentType = type;
    
    document.getElementById('currentFileName').textContent = filename;
    loadCurrentFile();
}

// 加载当前文件
async function loadCurrentFile() {
    if (!currentFile) return;
    
    const tailLines = document.getElementById('tailLines').value;
    const url = `/api/logs?file=${currentFile}&type=${currentType}${tailLines ? '&tail=' + tailLines : ''}`;
    
    try {
        const response = await fetch(url);
        const data = await response.json();
        
        allLogs = data.logs;
        
        const fileInfo = data.file_info;
        document.getElementById('fileInfo').textContent = 
            `(${fileInfo.type === 'client' ? '客户端' : '服务端'} | ${formatFileSize(fileInfo.file_size)} | ${fileInfo.total_lines} 行)`;
        
        filterLogs();
        updateLastUpdate();
    } catch (error) {
        console.error('加载日志失败:', error);
        showError('加载日志文件失败');
    }
}

// 过滤日志
function filterLogs() {
    const searchText = document.getElementById('searchBox').value.toLowerCase();
    const frameType = document.getElementById('frameType').value;
    const direction = document.getElementById('direction').value;
    
    filteredLogs = allLogs.filter(log => {
        // 方向过滤
        if (direction && log.direction !== direction) {
            return false;
        }
        
        // 帧类型过滤
        if (frameType && log.frame_info && log.frame_info.type !== frameType) {
            return false;
        }
        
        // 文本搜索
        if (searchText) {
            const searchIn = (
                log.data + ' ' + 
                (log.frame_info ? JSON.stringify(log.frame_info) : '')
            ).toLowerCase();
            if (!searchIn.includes(searchText)) {
                return false;
            }
        }
        
        return true;
    });
    
    displayLogs();
}

// 显示日志
function displayLogs() {
    const container = document.getElementById('logContainer');
    
    if (filteredLogs.length === 0) {
        if (allLogs.length === 0) {
            container.innerHTML = `
                <div class="welcome-message">
                    <h2>暂无日志数据</h2>
                    <p>该文件可能为空或格式不正确</p>
                </div>
            `;
        } else {
            container.innerHTML = `
                <div class="welcome-message">
                    <h2>没有匹配的日志</h2>
                    <p>尝试调整筛选条件</p>
                </div>
            `;
        }
        updateCounts();
        return;
    }
    
    let html = '<table class="log-table"><thead><tr>' +
        '<th>#</th>' +
        '<th>时间</th>' +
        '<th>方向</th>' +
        '<th>长度</th>' +
        '<th>帧类型</th>' +
        '<th>帧信息</th>' +
        '<th>数据</th>' +
        '<th>操作</th>' +
        '</tr></thead><tbody>';
    
    filteredLogs.forEach(log => {
        const frameInfo = log.frame_info || {};
        
        html += `
            <tr class="log-row">
                <td class="line-number">${log.line_num}</td>
                <td class="timestamp">${log.timestamp}</td>
                <td><span class="direction-badge ${log.direction}">${log.direction_desc || log.direction}</span></td>
                <td class="data-length">${log.length} B</td>
                <td class="frame-type">
                    ${frameInfo.type ? `
                        <span class="frame-badge frame-${frameInfo.type}">
                            ${frameInfo.type}帧
                        </span>
                    ` : '-'}
                </td>
                <td class="frame-details">
                    ${formatFrameInfo(frameInfo)}
                </td>
                <td class="data-hex">
                    <code>${formatHexData(log.data)}</code>
                </td>
                <td>
                    <button class="btn-detail" onclick='showLogDetail(${JSON.stringify(log).replace(/'/g, "\\'")})'>
                        详情
                    </button>
                </td>
            </tr>
        `;
    });
    
    html += '</tbody></table>';
    container.innerHTML = html;
    updateCounts();
}

// 格式化帧信息
function formatFrameInfo(frameInfo) {
    if (!frameInfo || !frameInfo.type) return '-';
    
    let info = [];
    
    if (frameInfo.type === 'I') {
        info.push(`<span class="info-item">发送序号: ${frameInfo.send_seq}</span>`);
        info.push(`<span class="info-item">接收序号: ${frameInfo.recv_seq}</span>`);
        if (frameInfo.type_id_desc) {
            info.push(`<span class="info-item type-id">${frameInfo.type_id_desc}</span>`);
        }
        if (frameInfo.cause_desc) {
            info.push(`<span class="info-item">原因: ${frameInfo.cause_desc}</span>`);
        }
        if (frameInfo.num_obj) {
            info.push(`<span class="info-item">对象数: ${frameInfo.num_obj}</span>`);
        }
    } else if (frameInfo.type === 'S') {
        info.push(`<span class="info-item">接收序号: ${frameInfo.recv_seq}</span>`);
    } else if (frameInfo.type === 'U') {
        info.push(`<span class="info-item function">${frameInfo.function}</span>`);
    }
    
    return info.join(' ');
}

// 格式化十六进制数据
function formatHexData(data) {
    if (!data) return '';
    
    // 限制显示长度
    const maxLength = 48;
    let formatted = data;
    
    if (data.length > maxLength) {
        formatted = data.substring(0, maxLength) + '...';
    }
    
    // 高亮IEC104起始字符
    formatted = formatted.replace(/^68/, '<span class="hex-highlight">68</span>');
    
    return formatted;
}

// 显示日志详情
function showLogDetail(log) {
    const modal = document.createElement('div');
    modal.className = 'modal';
    modal.innerHTML = `
        <div class="modal-content">
            <span class="modal-close" onclick="this.parentElement.parentElement.remove()">&times;</span>
            <h2>日志详情</h2>
            <div class="detail-section">
                <h3>基本信息</h3>
                <table class="detail-table">
                    <tr><td>时间戳:</td><td>${log.timestamp}</td></tr>
                    <tr><td>方向:</td><td>${log.direction_desc || log.direction}</td></tr>
                    <tr><td>数据长度:</td><td>${log.length} 字节</td></tr>
                </table>
            </div>
            
            ${log.frame_info ? `
            <div class="detail-section">
                <h3>IEC104帧解析</h3>
                <table class="detail-table">
                    <tr><td>帧类型:</td><td>${log.frame_info.type_desc || log.frame_info.type}</td></tr>
                    ${log.frame_info.type === 'I' ? `
                        <tr><td>发送序号:</td><td>${log.frame_info.send_seq}</td></tr>
                        <tr><td>接收序号:</td><td>${log.frame_info.recv_seq}</td></tr>
                        <tr><td>类型标识:</td><td>${log.frame_info.type_id} - ${log.frame_info.type_id_desc || ''}</td></tr>
                        <tr><td>传送原因:</td><td>${log.frame_info.cause} - ${log.frame_info.cause_desc || ''}</td></tr>
                        <tr><td>ASDU地址:</td><td>${log.frame_info.asdu_addr}</td></tr>
                        <tr><td>对象数量:</td><td>${log.frame_info.num_obj}</td></tr>
                    ` : ''}
                    ${log.frame_info.type === 'S' ? `
                        <tr><td>接收序号:</td><td>${log.frame_info.recv_seq}</td></tr>
                    ` : ''}
                    ${log.frame_info.type === 'U' ? `
                        <tr><td>功能:</td><td>${log.frame_info.function}</td></tr>
                    ` : ''}
                    <tr><td>控制域:</td><td><code>${log.frame_info.ctrl}</code></td></tr>
                </table>
            </div>
            ` : ''}
            
            <div class="detail-section">
                <h3>原始数据 (十六进制)</h3>
                <div class="hex-display">${formatHexDisplay(log.data)}</div>
            </div>
            
            <div class="detail-section">
                <h3>原始日志</h3>
                <pre class="raw-log">${log.raw}</pre>
            </div>
        </div>
    `;
    document.body.appendChild(modal);
}

// 格式化十六进制显示
function formatHexDisplay(data) {
    if (!data) return '';
    
    const bytes = data.trim().split(' ');
    let html = '<div class="hex-grid">';
    
    bytes.forEach((byte, index) => {
        // 特殊字节高亮
        let className = 'hex-byte';
        if (index === 0 && byte === '68') className += ' hex-start';
        else if (index === 1) className += ' hex-length';
        else if (index >= 2 && index <= 5) className += ' hex-control';
        else if (index === 6) className += ' hex-type-id';
        
        html += `<span class="${className}" title="字节 ${index}">${byte}</span>`;
        
        // 每16字节换行
        if ((index + 1) % 16 === 0) {
            html += '<br>';
        }
    });
    
    html += '</div>';
    return html;
}

// 更新计数
function updateCounts() {
    document.getElementById('logCount').textContent = `共 ${allLogs.length} 条日志`;
    if (filteredLogs.length !== allLogs.length) {
        document.getElementById('filterCount').textContent = `(筛选后 ${filteredLogs.length} 条)`;
    } else {
        document.getElementById('filterCount').textContent = '';
    }
}

// 更新最后更新时间
function updateLastUpdate() {
    const now = new Date();
    document.getElementById('lastUpdate').textContent = 
        `最后更新: ${now.toLocaleTimeString('zh-CN')}`;
}

// 其他功能函数保持不变...
function refreshFiles() {
    loadFileList();
    if (currentFile) {
        loadCurrentFile();
    }
}

function toggleAutoRefresh() {
    const checkbox = document.getElementById('autoRefresh');
    if (checkbox.checked) {
        autoRefreshInterval = setInterval(() => {
            if (currentFile) {
                loadCurrentFile();
            }
        }, 5000);
    } else {
        clearInterval(autoRefreshInterval);
        autoRefreshInterval = null;
    }
}

function exportLogs() {
    if (filteredLogs.length === 0) {
        alert('没有可导出的日志');
        return;
    }
    
    let content = 'Time\tDirection\tLength\tFrameType\tData\n';
    filteredLogs.forEach(log => {
        const frameType = log.frame_info ? log.frame_info.type : '-';
        content += `${log.timestamp}\t${log.direction}\t${log.length}\t${frameType}\t${log.data}\n`;
    });
    
    const blob = new Blob([content], { type: 'text/plain;charset=utf-8' });
    const url = window.URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `iec104_logs_${new Date().getTime()}.txt`;
    a.click();
    window.URL.revokeObjectURL(url);
}

function clearDisplay() {
    document.getElementById('logContainer').innerHTML = `
        <div class="welcome-message">
            <h2>显示已清空</h2>
            <p>选择文件重新加载日志</p>
        </div>
    `;
    allLogs = [];
    filteredLogs = [];
    updateCounts();
}

function showError(message) {
    document.getElementById('logContainer').innerHTML = `
        <div class="welcome-message" style="color: #e74c3c;">
            <h2>错误</h2>
            <p>${message}</p>
        </div>
    `;
}