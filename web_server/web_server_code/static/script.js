// API 基础路径
const API_BASE = '';

// 当前选中的文件
let currentFile = null;
let currentType = null;

// 初始化
document.addEventListener('DOMContentLoaded', () => {
    loadFiles();
    
    // 事件监听
    document.getElementById('refreshBtn').addEventListener('click', () => {
        if (currentFile) {
            loadLogs(currentFile, currentType);
        } else {
            loadFiles();
        }
    });
    
    document.getElementById('statsBtn').addEventListener('click', showStats);
    document.getElementById('tailLines').addEventListener('change', () => {
        if (currentFile) loadLogs(currentFile, currentType);
    });
    
    document.getElementById('frameFilter').addEventListener('change', () => {
        if (currentFile) loadLogs(currentFile, currentType);
    });
    
    document.getElementById('searchInput').addEventListener('input', filterLogs);
    
    // 模态框关闭
    document.querySelector('.close').addEventListener('click', () => {
        document.getElementById('statsModal').style.display = 'none';
    });
});

// 加载文件列表
async function loadFiles() {
    try {
        const response = await fetch(`${API_BASE}/api/files`);
        const data = await response.json();
        
        if (!data.success) {
            throw new Error(data.error);
        }
        
        renderFileList('clientFiles', data.data.client, 'client');
        renderFileList('serverFiles', data.data.server, 'server');
        
    } catch (error) {
        console.error('加载文件列表失败:', error);
        alert('加载文件列表失败: ' + error.message);
    }
}

// 渲染文件列表
function renderFileList(containerId, files, type) {
    const container = document.getElementById(containerId);
    
    if (files.length === 0) {
        container.innerHTML = '<p class="no-files">暂无日志文件</p>';
        return;
    }
    
    container.innerHTML = files.map(file => `
        <div class="file-item" data-file="${file.name}" data-type="${type}">
            <div class="file-name">${file.name}</div>
            <div class="file-meta">
                <span>${file.size_formatted}</span>
                <span>${file.modified}</span>
            </div>
        </div>
    `).join('');
    
    // 添加点击事件
    container.querySelectorAll('.file-item').forEach(item => {
        item.addEventListener('click', () => {
            const fileName = item.dataset.file;
            const fileType = item.dataset.type;
            
            // 更新选中状态
            document.querySelectorAll('.file-item').forEach(el => el.classList.remove('active'));
            item.classList.add('active');
            
            loadLogs(fileName, fileType);
        });
    });
}

// 加载日志内容
async function loadLogs(fileName, fileType) {
    try {
        currentFile = fileName;
        currentType = fileType;
        
        const tailLines = document.getElementById('tailLines').value;
        const frameFilter = document.getElementById('frameFilter').value;
        
        let url = `${API_BASE}/api/logs?file=${encodeURIComponent(fileName)}&type=${fileType}`;
        if (tailLines) url += `&tail=${tailLines}`;
        if (frameFilter) url += `&filter=${frameFilter}`;
        
        const response = await fetch(url);
        const data = await response.json();
        
        if (!data.success) {
            throw new Error(data.error);
        }
        
        renderFileInfo(data.file_info);
        renderLogs(data.logs);
        
    } catch (error) {
        console.error('加载日志失败:', error);
        alert('加载日志失败: ' + error.message);
    }
}

// 渲染文件信息
function renderFileInfo(info) {
    const container = document.getElementById('fileInfo');
    container.innerHTML = `
        <strong>${info.name}</strong>
        <span>类型: ${info.type === 'client' ? '客户端' : '服务端'}</span>
        <span>行数: ${info.total_lines}</span>
        <span>大小: ${info.file_size_formatted}</span>
        <span>修改时间: ${info.modified}</span>
    `;
}

// 渲染日志列表
function renderLogs(logs) {
    const container = document.getElementById('logContent');
    
    if (logs.length === 0) {
        container.innerHTML = '<div class="empty-state"><p>没有找到日志记录</p></div>';
        return;
    }
    
    container.innerHTML = `
        <table class="log-table">
            <thead>
                <tr>
                    <th>时间</th>
                    <th>方向</th>
                    <th>帧类型</th>
                    <th>详细信息</th>
                    <th>原始数据</th>
                </tr>
            </thead>
            <tbody>
                ${logs.map(log => renderLogRow(log)).join('')}
            </tbody>
        </table>
    `;
}

// 渲染单行日志
function renderLogRow(log) {
    const frame = log.frame_info;
    const dirClass = log.direction === 'TX' ? 'dir-tx' : 'dir-rx';
    const frameClass = `frame-${(frame.type || 'UNKNOWN').toLowerCase()}`;
    
    let details = '';
    
    if (frame.type === 'I') {
        details = `
            <div><strong>发送序号:</strong> ${frame.send_seq}</div>
            <div><strong>接收序号:</strong> ${frame.recv_seq}</div>
            ${frame.type_id_desc ? `<div><strong>类型:</strong> ${frame.type_id_desc}</div>` : ''}
            ${frame.cause_desc ? `<div><strong>原因:</strong> ${frame.cause_desc}</div>` : ''}
            ${frame.asdu_addr !== undefined ? `<div><strong>ASDU地址:</strong> ${frame.asdu_addr}</div>` : ''}
            ${frame.ioa !== undefined ? `<div><strong>IOA:</strong> ${frame.ioa}</div>` : ''}
            ${frame.value !== undefined ? `<div><strong>值:</strong> ${frame.value}</div>` : ''}
        `;
    } else if (frame.type === 'S') {
        details = `<div><strong>接收序号:</strong> ${frame.recv_seq}</div>`;
    } else if (frame.type === 'U') {
        details = `<div><strong>功能:</strong> ${frame.function}</div>`;
    } else {
        details = `<div>${frame.description || '无详细信息'}</div>`;
    }
    
    return `
        <tr class="log-row ${dirClass} ${frameClass}">
            <td class="timestamp">${log.timestamp}</td>
            <td class="direction">
                <span class="badge ${dirClass}">${log.direction_desc}</span>
            </td>
            <td class="frame-type">
                <span class="badge ${frameClass}">${frame.type_desc || frame.type || 'N/A'}</span>
            </td>
            <td class="details">${details}</td>
            <td class="raw-data">
                <code>${log.data}</code>
            </td>
        </tr>
    `;
}

// 过滤日志
function filterLogs() {
    const searchText = document.getElementById('searchInput').value.toLowerCase();
    const rows = document.querySelectorAll('.log-row');
    
    rows.forEach(row => {
        const text = row.textContent.toLowerCase();
        row.style.display = text.includes(searchText) ? '' : 'none';
    });
}

// 显示统计信息
async function showStats() {
    if (!currentFile) {
        alert('请先选择日志文件');
        return;
    }
    
    try {
        const url = `${API_BASE}/api/stats?file=${encodeURIComponent(currentFile)}&type=${currentType}`;
        const response = await fetch(url);
        const data = await response.json();
        
        if (!data.success) {
            throw new Error(data.error);
        }
        
        renderStats(data.stats);
        document.getElementById('statsModal').style.display = 'block';
        
    } catch (error) {
        console.error('获取统计信息失败:', error);
        alert('获取统计信息失败: ' + error.message);
    }
}

// 渲染统计信息
function renderStats(stats) {
    const container = document.getElementById('statsContent');
    
    container.innerHTML = `
        <div class="stats-section">
            <h3>总体统计</h3>
            <p>总记录数: <strong>${stats.total}</strong></p>
        </div>
        
        <div class="stats-section">
            <h3>方向统计</h3>
            ${Object.entries(stats.directions).map(([key, value]) => 
                `<p>${key}: <strong>${value}</strong></p>`
            ).join('')}
        </div>
        
        <div class="stats-section">
            <h3>帧类型统计</h3>
            ${Object.entries(stats.frame_types).map(([key, value]) => 
                `<p>${key}帧: <strong>${value}</strong></p>`
            ).join('')}
        </div>
        
        ${Object.keys(stats.type_ids).length > 0 ? `
            <div class="stats-section">
                <h3>类型标识统计</h3>
                ${Object.entries(stats.type_ids).map(([key, value]) => 
                    `<p>${key}: <strong>${value}</strong></p>`
                ).join('')}
            </div>
        ` : ''}
        
        ${Object.keys(stats.causes).length > 0 ? `
            <div class="stats-section">
                <h3>传输原因统计</h3>
                ${Object.entries(stats.causes).map(([key, value]) => 
                    `<p>${key}: <strong>${value}</strong></p>`
                ).join('')}
            </div>
        ` : ''}
    `;
}