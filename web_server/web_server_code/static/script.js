// API 基础路径
const API_BASE = '';

// 当前选中的文件
let currentFile = null;
let currentType = null;

// 当前选中的文件
let autoRefresh = false;  // 添加
let refreshInterval = null;  // 添加
let lastTimestamp = 0;  // 添加：最后一条日志的时间戳

let lastFilePosition = 0; // 添加：文件读取位置

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
    // 修改：操作 wrapper 而不是按钮
    const logContent = document.getElementById('logContent');
    const scrollTopWrapper = document.querySelector('.scroll-top-wrapper');  // 修改
    const scrollTopBtn = document.getElementById('scrollTopBtn');

    if (logContent && scrollTopWrapper && scrollTopBtn) {
        // 滚动监听
        logContent.addEventListener('scroll', () => {
            if (logContent.scrollTop > 300) {
                scrollTopWrapper.classList.add('show');  // 修改
            } else {
                scrollTopWrapper.classList.remove('show');  // 修改
            }
        });

        // 点击回到顶部
        scrollTopBtn.addEventListener('click', () => {
            logContent.scrollTo({
                top: 0,
                behavior: 'smooth'
            });
        });
    }

    // 自动刷新按钮 - 确认这部分存在
    const autoRefreshBtn = document.getElementById('autoRefreshBtn');
    console.log('autoRefreshBtn 元素:', autoRefreshBtn);

    if (autoRefreshBtn) {
        autoRefreshBtn.addEventListener('click', () => {
            console.log('自动刷新按钮被点击');
            toggleAutoRefresh();
        });
        console.log('✅ 自动刷新按钮事件已绑定');
    } else {
        console.error('❌ 找不到自动刷新按钮！');
    }

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

        // 重置文件位置（重新加载时）
        lastFilePosition = data.file_info.file_size || 0;  // 添加：设置初始位置

    } catch (error) {
        console.error('加载日志失败:', error);
        alert('加载日志失败: ' + error.message);
    }
}


// 切换自动刷新
function toggleAutoRefresh() {
    console.log('=== toggleAutoRefresh 被调用 ===');

    autoRefresh = !autoRefresh;
    const btn = document.getElementById('autoRefreshBtn');
    const statusIndicator = document.getElementById('refreshStatus');

    if (autoRefresh) {
        btn.innerHTML = '⏸ 停止刷新<span id="refreshStatus" class="refresh-indicator idle"></span>';
        btn.classList.add('active');
        console.log('准备开始自动刷新...');
        startAutoRefresh();
    } else {
        btn.innerHTML = '▶ 自动刷新<span id="refreshStatus" class="refresh-indicator"></span>';
        btn.classList.remove('active');
        console.log('停止自动刷新');
        stopAutoRefresh();
    }
}

// 开始自动刷新
function startAutoRefresh() {
    console.log('=== startAutoRefresh 被调用 ===');
    console.log('currentFile:', currentFile);

    if (!currentFile) {
        alert('请先选择日志文件');
        autoRefresh = false;
        const btn = document.getElementById('autoRefreshBtn');
        btn.textContent = '▶ 自动刷新';
        btn.classList.remove('active');
        return;
    }

    console.log('设置定时器，每2秒执行一次');

    // 立即执行一次
    fetchNewLogs();

    // 每2秒刷新一次
    refreshInterval = setInterval(() => {
        console.log('定时器触发，执行 fetchNewLogs');
        fetchNewLogs();
    }, 2000);

    console.log('refreshInterval ID:', refreshInterval);
}

// 停止自动刷新
function stopAutoRefresh() {
    if (refreshInterval) {
        clearInterval(refreshInterval);
        refreshInterval = null;
    }
}

// 获取新日志
async function fetchNewLogs() {
    console.log('=== fetchNewLogs 开始执行 ===');

    // 添加：显示刷新指示器
    const statusIndicator = document.getElementById('refreshStatus');
    if (statusIndicator) {
        statusIndicator.className = 'refresh-indicator loading';
    }

    if (!currentFile) {
        console.log('没有选择文件，返回');
        return;
    }

    try {
        const url = `${API_BASE}/api/logs/tail?file=${encodeURIComponent(currentFile)}&type=${currentType}&since=${lastTimestamp}&position=${lastFilePosition}`;
        console.log('请求 URL:', url);

        const response = await fetch(url);
        console.log('响应状态:', response.status);

        const data = await response.json();
        console.log('返回数据:', data);

        if (data.success && data.logs.length > 0) {
            console.log(`✅ 获取到 ${data.logs.length} 条新日志`);

            // 添加：绿色闪烁（有新数据）
            if (statusIndicator) {
                statusIndicator.className = 'refresh-indicator active';
            }

            appendLogs(data.logs);
            lastTimestamp = Math.max(...data.logs.map(log => log.timestamp_ms));
        } else {
            console.log('📭 没有新日志');

            // 添加：灰色（无新数据）
            if (statusIndicator) {
                statusIndicator.className = 'refresh-indicator idle';
            }
        }

        // 更新文件读取位置
        if (data.position !== undefined) {
            console.log('更新位置:', lastFilePosition, '->', data.position);
            lastFilePosition = data.position;
        }

    } catch (error) {
        console.error('❌ 获取新日志失败:', error);

        // 添加：红色（出错）
        if (statusIndicator) {
            statusIndicator.className = 'refresh-indicator error';
        }
    }

    // 修改：0.5秒后变回空闲状态
    setTimeout(() => {
        if (statusIndicator && autoRefresh) {
            statusIndicator.className = 'refresh-indicator idle';
        } else if (statusIndicator) {
            statusIndicator.className = 'refresh-indicator';  // 透明
        }
    }, 500);
}

// 追加日志到表格
function appendLogs(logs) {
    const tbody = document.querySelector('.log-table tbody');
    if (!tbody) return;

    logs.forEach(log => {
        const row = document.createElement('tr');
        row.innerHTML = renderLogRow(log).replace(/<\/?tr[^>]*>/g, '');
        row.classList.add('new-log');  // 添加高亮类
        tbody.insertBefore(row, tbody.firstChild);

        // 0.5秒后移除高亮
        setTimeout(() => row.classList.remove('new-log'), 500);
    });

    // 自动滚动到底部
    const container = document.getElementById('logContent');
    container.scrollTop = container.scrollHeight;
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
// 渲染日志列表
function renderLogs(logs) {
    const container = document.getElementById('logContent');

    if (logs.length === 0) {
        container.innerHTML = '<div class="empty-state"><p>没有找到日志记录</p></div>';
        lastTimestamp = 0;  // 添加
        lastFilePosition = 0;  // 添加：重置位置
        return;
    }

    // 更新最后时间戳
    lastTimestamp = Math.max(...logs.map(log => log.timestamp_ms));  // 添加

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