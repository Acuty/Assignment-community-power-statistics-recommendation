'use strict';

/* 保持现有 C 后端接口路径、参数和数据格式。 */
const API = {
  health: '/api/health',
  login: '/api/login',
  records: '/api/records',
  statistics: null,
  sorting: null,
  recommend: '/api/recommend'
};
const PAGE_SIZE = 50;
const MAX_FRONTEND_RECORDS = 40000;
const MAX_PAGES = Math.ceil(MAX_FRONTEND_RECORDS / PAGE_SIZE);
const state = {
  account: '',
  password: '',
  role: 0,
  page: 1,
  count: 0,
  rows: [],
  editing: null,
  timer: 0,
  reportRows: [],
  chartType: 'line'
};
const $ = id => document.getElementById(id);
const pageTitles = {
  overview: '数据总览', entry: '数据维护', records: '用电查询',
  reports: '统计报表', sorting: '数据排序', recommend: '扩容推荐'
};

function message(id, text, kind) {
  const node = $(id);
  if (!node) return;
  node.textContent = text || '';
  node.className = 'message' + (kind ? ' ' + kind : '');
}
function toast(text) {
  const node = $('toast');
  node.textContent = text;
  node.classList.add('show');
  clearTimeout(state.timer);
  state.timer = setTimeout(() => node.classList.remove('show'), 2400);
}
function escapeHtml(value) {
  return String(value == null ? '' : value).replace(/[&<>"']/g, ch => ({
    '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;'
  }[ch]));
}
function queryString(values) {
  const parts = [];
  Object.keys(values || {}).forEach(key => {
    if (values[key] !== '' && values[key] != null) {
      parts.push(encodeURIComponent(key) + '=' + encodeURIComponent(values[key]));
    }
  });
  return parts.length ? '?' + parts.join('&') : '';
}
function basicAuthorization() {
  return 'Basic ' + btoa(unescape(encodeURIComponent(state.account + ':' + state.password)));
}
async function api(method, path, options) {
  const opt = options || {};
  const headers = {};
  const config = { method, headers };
  if (opt.body !== undefined) {
    headers['Content-Type'] = 'application/json';
    config.body = JSON.stringify(opt.body);
  }
  if (opt.auth) headers.Authorization = basicAuthorization();

  let response;
  try {
    response = await fetch(path + queryString(opt.params), config);
  } catch (_) {
    throw new Error('无法连接服务，请先启动 C 程序。');
  }
  const text = await response.text();
  let data = null;
  if (text) {
    try { data = JSON.parse(text); } catch (_) { /* 下面统一报告格式错误 */ }
  }
  if (!response.ok) throw new Error(data && data.error ? data.error : '请求失败（' + response.status + '）');
  if (text && data === null) throw new Error('服务器返回数据格式无效。');
  return data;
}

async function checkHealth() {
  try {
    const result = await api('GET', API.health);
    const online = result && result.status === 'ok';
    $('login-status').textContent = online ? '服务已连接' : '服务异常';
    $('login-dot').classList.toggle('online', online);
    $('status').textContent = online ? '服务运行正常' : '服务异常';
    $('side-status').textContent = online ? '服务运行正常' : '服务异常';
    $('side-dot').classList.toggle('online', online);
  } catch (_) {
    $('login-status').textContent = '服务未连接，请启动程序';
    $('login-dot').classList.remove('online');
    $('status').textContent = '服务未连接';
    $('side-status').textContent = '服务未连接';
    $('side-dot').classList.remove('online');
  }
}

function showWorkspace() {
  const loggedIn = state.role === 1 || state.role === 2;
  $('login-screen').classList.toggle('hidden', loggedIn);
  $('app-shell').classList.toggle('hidden', !loggedIn);
  document.body.classList.toggle('is-admin', state.role === 1);
  document.querySelectorAll('.auth-only').forEach(node => node.classList.toggle('hidden', !loggedIn));
  document.querySelectorAll('.admin-only').forEach(node => node.classList.toggle('hidden', state.role !== 1));
  $('top-login').textContent = state.account ? '退出登录' : '登录';
  $('side-login').textContent = state.account ? state.account + ' · 退出登录 ↗' : '退出登录 ↗';
  const profileName = $('profile-name');
  const profileRole = $('profile-role');
  if (profileName) profileName.textContent = state.account || '';
  if (profileRole) profileRole.textContent = state.role === 1 ? '管理员' : '普通用户';
}
function go(page) {
  if (!state.role) return;
  if (page === 'entry' && state.role !== 1) {
    toast('数据维护仅对管理员开放。');
    return;
  }
  document.querySelectorAll('.page').forEach(node => {
    node.classList.toggle('active', node.dataset.view === page);
  });
  document.querySelectorAll('.nav[data-page]').forEach(node => {
    node.classList.toggle('active', node.dataset.page === page);
  });
  $('page-title').textContent = pageTitles[page] || '数据总览';
  window.scrollTo(0, 0);
}

async function login(event) {
  if (event) event.preventDefault();
  const account = $('login-account').value.trim();
  const password = $('login-password').value;
  if (!account || !password) return message('login-message', '请输入账号和密码。', 'error');

  $('login-submit').disabled = true;
  message('login-message', '正在验证账号…', 'info');
  try {
    const result = await api('POST', API.login, { body: { account, password } });
    state.account = result.account;
    state.password = password;
    state.role = Number(result.role) || 0;
    showWorkspace();
    go('overview');
    $('login-password').value = '';
    message('login-message', '', '');
    toast('登录成功，已进入工作台。');
  } catch (error) {
    message('login-message', error.message, 'error');
  } finally {
    $('login-submit').disabled = false;
  }
}
function clearWorkspace() {
  state.rows = [];
  state.page = 1;
  state.count = 0;
  state.reportRows = [];
  state.editing = null;
  $('records').innerHTML = '<tr><td class="empty-cell" colspan="8">登录后查询记录。</td></tr>';
  $('recent').innerHTML = '<tr><td class="empty-cell" colspan="5">暂无数据</td></tr>';
  $('report-rows').innerHTML = '<tr><td class="empty-cell" colspan="8">等待报表数据。</td></tr>';
  $('sort-rows').innerHTML = '<tr><td class="empty-cell" colspan="9">选择小区后开始排序。</td></tr>';
  $('recommend-rows').innerHTML = '<tr><td class="empty-cell" colspan="10">登录后生成推荐名单。</td></tr>';
  ['metric-records', 'metric-kwh', 'metric-bill', 'metric-homes', 'r-peak', 'r-valley', 'r-bill', 'r-average']
    .forEach(id => { if ($(id)) $(id).textContent = '—'; });
  $('overview-chart').innerHTML = '<div class="empty-state">输入小区名称并读取数据后显示趋势图。</div>';
  $('chart').innerHTML = '<div class="empty-state">填写小区并生成报表后显示统计图。</div>';
  $('snapshot-note').textContent = '读取数据后显示记录';
  $('result-count').textContent = '尚未查询';
  $('overview-chart-caption').textContent = '读取数据后显示峰谷电量变化';
  $('chart-caption').textContent = '生成报表后选择图表形式';
}
function logout() {
  state.account = '';
  state.password = '';
  state.role = 0;
  clearWorkspace();
  showWorkspace();
  $('login-account').value = '';
  $('login-password').value = '';
  message('login-message', '', '');
  toast('已退出登录。');
}

function validateFilter(filter) {
  if (!filter.community) return '请填写小区名称。';
  if (filter.year && (!/^\d{1,4}$/.test(String(filter.year)) || Number(filter.year) < 1 || Number(filter.year) > 9999)) {
    return '年份格式无效。';
  }
  if (filter.month && (!/^\d{1,2}$/.test(String(filter.month)) || Number(filter.month) < 1 || Number(filter.month) > 12)) {
    return '月份须在 1 到 12 之间。';
  }
  return '';
}
function formatNumber(value, digits) {
  const number = Number(value);
  if (!Number.isFinite(number)) return '0';
  const decimals = digits === undefined ? 2 : digits;
  return number.toLocaleString('zh-CN', { minimumFractionDigits: decimals, maximumFractionDigits: decimals });
}
async function getPage(filter, page) {
  const rows = await api('GET', API.records, {
    params: {
      community: filter.community,
      household: filter.household,
      year: filter.year,
      month: filter.month,
      page
    },
    auth: true
  });
  if (!Array.isArray(rows)) throw new Error('查询接口返回格式不正确。');
  return rows;
}
async function getAll(filter) {
  const allRows = [];
  for (let page = 1; page <= MAX_PAGES; page++) {
    const rows = await getPage(filter, page);
    allRows.push(...rows);
    if (rows.length < PAGE_SIZE) break;
  }
  return allRows;
}
function renderRecordRows(rows, targetId, admin) {
  const body = $(targetId);
  if (!rows.length) {
    body.innerHTML = '<tr><td class="empty-cell" colspan="' + (admin ? 8 : 7) + '">没有找到符合条件的记录。</td></tr>';
    return;
  }
  body.innerHTML = rows.map((row, index) => '<tr>' +
    '<td>' + escapeHtml(row.community) + '</td>' +
    '<td>' + escapeHtml(row.household) + '</td>' +
    '<td>' + escapeHtml(row.year) + '</td>' +
    '<td>' + escapeHtml(row.month) + '</td>' +
    '<td>' + formatNumber(row.peak_kwh) + '</td>' +
    '<td>' + formatNumber(row.valley_kwh) + '</td>' +
    '<td>¥ ' + formatNumber(row.bill) + '</td>' +
    (admin ? '<td class="admin-col"><button class="table-action" data-edit="' + index + '">修改</button>' +
      '<button class="table-action danger" data-del="' + index + '">删除</button></td>' : '') +
    '</tr>').join('');
  if (admin) {
    body.querySelectorAll('[data-edit]').forEach(button => {
      button.onclick = () => editRecord(rows[Number(button.dataset.edit)]);
    });
    body.querySelectorAll('[data-del]').forEach(button => {
      button.onclick = () => removeRecord(rows[Number(button.dataset.del)]);
    });
  }
}
function aggregate(rows) {
  const groups = new Map();
  rows.forEach(row => {
    const period = String(row.year) + '-' + String(row.month).padStart(2, '0');
    if (!groups.has(period)) groups.set(period, { period, peak: 0, valley: 0, bill: 0, homes: new Set() });
    const group = groups.get(period);
    group.peak += Number(row.peak_kwh) || 0;
    group.valley += Number(row.valley_kwh) || 0;
    group.bill += Number(row.bill) || 0;
    group.homes.add(row.household);
  });
  return Array.from(groups.values()).sort((a, b) => a.period.localeCompare(b.period)).map(group => ({
    period: group.period,
    homes: group.homes.size,
    peak: group.peak,
    valley: group.valley,
    total: group.peak + group.valley,
    bill: group.bill,
    avgKwh: group.homes.size ? (group.peak + group.valley) / group.homes.size : 0,
    avgBill: group.homes.size ? group.bill / group.homes.size : 0
  }));
}
function updateSnapshot(rows, community, label) {
  const peak = rows.reduce((sum, row) => sum + (Number(row.peak_kwh) || 0), 0);
  const valley = rows.reduce((sum, row) => sum + (Number(row.valley_kwh) || 0), 0);
  const bill = rows.reduce((sum, row) => sum + (Number(row.bill) || 0), 0);
  const households = new Set(rows.map(row => row.household)).size;
  $('metric-records').textContent = formatNumber(rows.length, 0);
  $('metric-records-note').textContent = label || community;
  $('metric-kwh').textContent = formatNumber(peak + valley);
  $('metric-bill').textContent = formatNumber(bill);
  $('metric-homes').textContent = formatNumber(households, 0);
  $('snapshot-note').textContent = community + (label ? ' · ' + label : '') + ' · 最近记录';
  const recent = rows.slice().sort((a, b) => (b.year * 100 + b.month) - (a.year * 100 + a.month)).slice(0, 6);
  $('recent').innerHTML = recent.length ? recent.map(row => '<tr>' +
    '<td><strong>' + escapeHtml(row.community) + '</strong><small>' + escapeHtml(row.household) + '</small></td>' +
    '<td>' + escapeHtml(row.year) + '/' + escapeHtml(row.month) + '</td>' +
    '<td>' + formatNumber(row.peak_kwh) + '</td>' +
    '<td>' + formatNumber(row.valley_kwh) + '</td>' +
    '<td>¥ ' + formatNumber(row.bill) + '</td></tr>').join('') :
    '<tr><td class="empty-cell" colspan="5">当前筛选下没有记录。</td></tr>';
}

function chartLine(rows) {
  if (!rows.length) return '<div class="empty-state">没有可显示的月度数据。</div>';
  const width = 760, height = 270, left = 52, right = 14, top = 18, bottom = 42;
  const plotWidth = width - left - right, plotHeight = height - top - bottom;
  const max = Math.max(1, ...rows.flatMap(row => [row.peak, row.valley]));
  const x = index => rows.length === 1 ? left + plotWidth / 2 : left + index * plotWidth / (rows.length - 1);
  const y = value => top + plotHeight - value / max * plotHeight;
  const pathFor = key => rows.map((row, index) => (index ? 'L' : 'M') + x(index).toFixed(1) + ' ' + y(row[key]).toFixed(1)).join(' ');
  const grid = [0, 1, 2, 3].map(i => {
    const yy = top + i * plotHeight / 3;
    const value = max * (1 - i / 3);
    return '<line x1="' + left + '" y1="' + yy + '" x2="' + (width - right) + '" y2="' + yy + '" class="grid-line"/>' +
      '<text x="' + (left - 8) + '" y="' + (yy + 4) + '" text-anchor="end" class="axis-label">' + formatNumber(value, 0) + '</text>';
  }).join('');
  const labelStep = Math.max(1, Math.ceil(rows.length / 8));
  const labels = rows.map((row, index) => index % labelStep === 0 || index === rows.length - 1
    ? '<text x="' + x(index) + '" y="' + (height - 13) + '" text-anchor="middle" class="axis-label">' + escapeHtml(row.period.slice(5)) + '</text>' : '').join('');
  const points = key => rows.map((row, index) => '<circle cx="' + x(index) + '" cy="' + y(row[key]) + '" r="3" class="point-' + key + '"/>').join('');
  return '<svg viewBox="0 0 ' + width + ' ' + height + '" role="img" aria-label="峰谷月度折线图">' + grid +
    '<path d="' + pathFor('peak') + '" class="line-peak"/><path d="' + pathFor('valley') + '" class="line-valley"/>' +
    points('peak') + points('valley') + labels + '</svg>';
}
function chartBars(rows) {
  if (!rows.length) return '<div class="empty-state">没有可显示的月度数据。</div>';
  const width = 760, height = 270, left = 52, right = 12, top = 20, bottom = 42;
  const plotWidth = width - left - right, plotHeight = height - top - bottom;
  const max = Math.max(1, ...rows.flatMap(row => [row.peak, row.valley]));
  const groupWidth = plotWidth / rows.length;
  const barWidth = Math.max(3, Math.min(18, groupWidth * .28));
  const grid = [0, 1, 2, 3].map(i => {
    const yy = top + i * plotHeight / 3;
    return '<line x1="' + left + '" y1="' + yy + '" x2="' + (width - right) + '" y2="' + yy + '" class="grid-line"/>' +
      '<text x="' + (left - 8) + '" y="' + (yy + 4) + '" text-anchor="end" class="axis-label">' + formatNumber(max * (1 - i / 3), 0) + '</text>';
  }).join('');
  const bars = rows.map((row, index) => {
    const center = left + index * groupWidth + groupWidth / 2;
    const peakHeight = row.peak / max * plotHeight;
    const valleyHeight = row.valley / max * plotHeight;
    const label = index % Math.max(1, Math.ceil(rows.length / 8)) === 0 || index === rows.length - 1
      ? '<text x="' + center + '" y="' + (height - 13) + '" text-anchor="middle" class="axis-label">' + escapeHtml(row.period.slice(5)) + '</text>' : '';
    return '<rect x="' + (center - barWidth - 1) + '" y="' + (top + plotHeight - peakHeight) + '" width="' + barWidth + '" height="' + peakHeight + '" rx="2" class="bar-peak"><title>' + escapeHtml(row.period) + ' 峰时 ' + formatNumber(row.peak) + ' kWh</title></rect>' +
      '<rect x="' + (center + 1) + '" y="' + (top + plotHeight - valleyHeight) + '" width="' + barWidth + '" height="' + valleyHeight + '" rx="2" class="bar-valley"><title>' + escapeHtml(row.period) + ' 谷时 ' + formatNumber(row.valley) + ' kWh</title></rect>' + label;
  }).join('');
  return '<svg viewBox="0 0 ' + width + ' ' + height + '" role="img" aria-label="峰谷月度柱状图">' + grid + bars + '</svg>';
}
function chartPie(rows) {
  if (!rows.length) return '<div class="empty-state">没有可显示的月度数据。</div>';
  const peak = rows.reduce((sum, row) => sum + row.peak, 0);
  const valley = rows.reduce((sum, row) => sum + row.valley, 0);
  const total = peak + valley;
  if (total <= 0) return '<div class="empty-state">当前统计周期的电量为 0，无法计算占比。</div>';
  const peakPercent = peak / total * 100;
  return '<div class="pie-view"><div class="donut" style="--peak-share:' + peakPercent + '%"><div><strong>' + formatNumber(total, 0) + '</strong><small>总用电量 · kWh</small></div></div>' +
    '<div class="pie-legend"><h3>峰谷用电构成</h3><div><i class="swatch peak-swatch"></i><span>峰时电量</span><strong>' + formatNumber(peak, 2) + ' kWh</strong><small>' + formatNumber(peakPercent, 1) + '%</small></div>' +
    '<div><i class="swatch valley-swatch"></i><span>谷时电量</span><strong>' + formatNumber(valley, 2) + ' kWh</strong><small>' + formatNumber(100 - peakPercent, 1) + '%</small></div></div></div>';
}
function chartTable(rows) {
  if (!rows.length) return '<div class="empty-state">没有可显示的月度数据。</div>';
  return '<div class="chart-table-wrap"><table class="chart-table"><thead><tr><th>月份</th><th>峰时用电</th><th>谷时用电</th><th>总电量</th><th>电费</th></tr></thead><tbody>' +
    rows.map(row => '<tr><td>' + escapeHtml(row.period) + '</td><td>' + formatNumber(row.peak) + '</td><td>' + formatNumber(row.valley) + '</td><td>' + formatNumber(row.total) + '</td><td>¥ ' + formatNumber(row.bill) + '</td></tr>').join('') +
    '</tbody></table></div>';
}
function drawChart(targetId, rows, type) {
  const target = $(targetId);
  if (!target) return;
  if (type === 'bar') target.innerHTML = chartBars(rows);
  else if (type === 'pie') target.innerHTML = chartPie(rows);
  else if (type === 'table') target.innerHTML = chartTable(rows);
  else target.innerHTML = chartLine(rows);
  target.classList.toggle('table-chart-mode', type === 'table');
}
function renderReportRows(rows) {
  $('report-rows').innerHTML = rows.length ? rows.map(row => '<tr><td>' + escapeHtml(row.period) + '</td><td>' + formatNumber(row.homes, 0) + '</td><td>' + formatNumber(row.peak) + '</td><td>' + formatNumber(row.valley) + '</td><td>' + formatNumber(row.total) + '</td><td>¥ ' + formatNumber(row.bill) + '</td><td>' + formatNumber(row.avgKwh) + '</td><td>¥ ' + formatNumber(row.avgBill) + '</td></tr>').join('') :
    '<tr><td class="empty-cell" colspan="8">没有统计数据。</td></tr>';
}
function renderReportChart() {
  drawChart('chart', state.reportRows, state.chartType);
  document.querySelectorAll('[data-chart-type]').forEach(button => {
    button.classList.toggle('active', button.dataset.chartType === state.chartType);
  });
}

async function loadOverview() {
  const filter = {
    community: $('overview-query-community').value.trim(),
    household: '', year: $('overview-query-year').value.trim(), month: ''
  };
  const error = validateFilter(filter);
  if (error) return toast(error);
  $('overview-load').disabled = true;
  try {
    const rows = await getAll(filter);
    const label = filter.year ? filter.year + ' 年' : '全部年份';
    updateSnapshot(rows, filter.community, label);
    $('overview-chart-caption').textContent = filter.community + ' · ' + label + ' · 峰谷电量';
    drawChart('overview-chart', aggregate(rows), 'line');
  } catch (error) {
    toast(error.message);
  } finally {
    $('overview-load').disabled = false;
  }
}
async function queryRecords(page) {
  if (!state.role) return;
  const filter = {
    community: $('q-community').value.trim(), household: $('q-household').value.trim(),
    year: $('q-year').value.trim(), month: $('q-month').value.trim()
  };
  const error = validateFilter(filter);
  if (error) return message('query-message', error, 'error');
  state.page = page || 1;
  $('query-btn').disabled = true;
  message('query-message', '正在读取…', 'info');
  try {
    const rows = await getPage(filter, state.page);
    state.rows = rows;
    state.count = rows.length;
    renderRecordRows(rows, 'records', state.role === 1);
    $('result-count').textContent = rows.length + ' 条记录';
    $('page-no').textContent = '第 ' + state.page + ' 页';
    $('prev').disabled = state.page <= 1;
    $('next').disabled = rows.length !== PAGE_SIZE;
    updateSnapshot(rows, filter.community, '当前查询页');
    message('query-message', rows.length ? '查询完成。' : '没有匹配记录。', rows.length ? 'success' : 'info');
  } catch (error) {
    message('query-message', error.message, 'error');
  } finally {
    $('query-btn').disabled = false;
  }
}
function editRecord(row) {
  state.editing = { community: row.community, household: row.household, year: row.year, month: row.month };
  $('entry-title').textContent = '修改用电记录';
  $('f-community').value = row.community;
  $('f-household').value = row.household;
  $('f-year').value = row.year;
  $('f-month').value = row.month;
  $('f-peak').value = row.peak_kwh;
  $('f-valley').value = row.valley_kwh;
  $('save-entry').textContent = '保存修改';
  $('cancel-edit').classList.remove('hidden');
  go('entry');
}
function clearEntry() {
  state.editing = null;
  $('entry-title').textContent = '新增用电记录';
  $('save-entry').textContent = '保存记录';
  $('cancel-edit').classList.add('hidden');
  ['f-community', 'f-household', 'f-year', 'f-month', 'f-peak', 'f-valley'].forEach(id => { $(id).value = ''; });
}
async function saveRecord() {
  if (state.role !== 1) return;
  const row = {
    community: $('f-community').value.trim(), household: $('f-household').value.trim(),
    year: Number($('f-year').value), month: Number($('f-month').value),
    peak_kwh: Number($('f-peak').value), valley_kwh: Number($('f-valley').value)
  };
  if (!row.community || !row.household) return message('entry-message', '小区和住户不能为空。', 'error');
  if (!Number.isInteger(row.year) || row.year < 1 || row.year > 9999 || !Number.isInteger(row.month) || row.month < 1 || row.month > 12) {
    return message('entry-message', '年份或月份格式无效。', 'error');
  }
  if (!Number.isFinite(row.peak_kwh) || row.peak_kwh < 0 || !Number.isFinite(row.valley_kwh) || row.valley_kwh < 0) {
    return message('entry-message', '电量必须为非负数。', 'error');
  }
  $('save-entry').disabled = true;
  try {
    if (state.editing) {
      await api('PUT', API.records, { params: state.editing, body: row, auth: true });
    } else {
      await api('POST', API.records, { body: row, auth: true });
    }
    $('overview-query-community').value = row.community;
    $('q-community').value = row.community;
    $('q-household').value = row.household;
    $('q-year').value = row.year;
    $('q-month').value = row.month;
    clearEntry();
    toast('记录保存成功，电费由后端计算。');
    go('records');
    await queryRecords(1);
  } catch (error) {
    message('entry-message', error.message, 'error');
  } finally {
    $('save-entry').disabled = false;
  }
}
async function removeRecord(row) {
  const detail = row.community + ' / ' + row.household + ' / ' + row.year + '年' + row.month + '月';
  if (state.role !== 1 || !window.confirm('确认删除这条记录？\n' + detail)) return;
  try {
    await api('DELETE', API.records, {
      params: { community: row.community, household: row.household, year: row.year, month: row.month }, auth: true
    });
    toast('记录已删除。');
    await queryRecords(state.page);
  } catch (error) {
    message('query-message', error.message, 'error');
  }
}

async function report() {
  if (!state.role) return;
  const filter = { community: $('report-community').value.trim(), household: '', year: $('report-year').value.trim(), month: '' };
  const error = validateFilter(filter);
  if (error) return message('report-message', error, 'error');
  $('report-btn').disabled = true;
  message('report-message', '正在读取全部分页…', 'info');
  try {
    const rows = await getAll(filter);
    state.reportRows = aggregate(rows);
    const peak = rows.reduce((sum, row) => sum + (Number(row.peak_kwh) || 0), 0);
    const valley = rows.reduce((sum, row) => sum + (Number(row.valley_kwh) || 0), 0);
    const billTotal = rows.reduce((sum, row) => sum + (Number(row.bill) || 0), 0);
    const households = new Set(rows.map(row => row.household)).size;
    $('r-peak').textContent = formatNumber(peak);
    $('r-valley').textContent = formatNumber(valley);
    $('r-bill').textContent = formatNumber(billTotal);
    $('r-average').textContent = formatNumber(households ? billTotal / households : 0);
    $('chart-caption').textContent = filter.community + (filter.year ? ' · ' + filter.year + ' 年' : ' · 全部年份');
    renderReportRows(state.reportRows);
    renderReportChart();
    message('report-message', rows.length ? '已统计 ' + rows.length + ' 条记录，覆盖 ' + households + ' 户。' : '当前条件没有记录。', rows.length ? 'success' : 'info');
  } catch (error) {
    message('report-message', error.message, 'error');
  } finally {
    $('report-btn').disabled = false;
  }
}
async function sortData() {
  if (!state.role) return;
  const filter = { community: $('sort-community').value.trim(), household: '', year: $('sort-year').value.trim(), month: '' };
  const error = validateFilter(filter);
  if (error) return message('sort-message', error, 'error');
  $('sort-btn').disabled = true;
  try {
    const rows = aggregate(await getAll(filter)).sort((a, b) => b[$('sort-metric').value] - a[$('sort-metric').value]);
    $('sort-count').textContent = rows.length + ' 个月度数据';
    $('sort-rows').innerHTML = rows.length ? rows.map((row, index) => '<tr><td><span class="rank ' + (index < 3 ? 'rank-top' : '') + '">' + (index + 1) + '</span></td><td>' + escapeHtml(row.period) + '</td><td>' + formatNumber(row.homes, 0) + '</td><td>' + formatNumber(row.peak) + '</td><td>' + formatNumber(row.valley) + '</td><td>' + formatNumber(row.total) + '</td><td>¥ ' + formatNumber(row.bill) + '</td><td>' + formatNumber(row.avgKwh) + '</td><td>¥ ' + formatNumber(row.avgBill) + '</td></tr>').join('') : '<tr><td colspan="9" class="empty-cell">没有排序数据。</td></tr>';
    message('sort-message', '已按指标从高到低排列。', rows.length ? 'success' : 'info');
  } catch (error) {
    message('sort-message', error.message, 'error');
  } finally {
    $('sort-btn').disabled = false;
  }
}
function renderRecommendations(rows) {
  if (!Array.isArray(rows)) throw new Error('推荐接口应返回 CommunityBill 数组。');
  $('recommend-rows').innerHTML = rows.length ? rows.map((row, index) => '<tr><td><span class="rank ' + (index === 0 ? 'rank-top' : '') + '">' + (index + 1) + '</span></td><td><strong>' + escapeHtml(row.community) + '</strong></td><td>' + escapeHtml(row.period_start_year) + '—' + escapeHtml(row.period_end_year) + '</td><td>' + formatNumber(row.total_peak_kwh) + '</td><td>' + formatNumber(row.total_valley_kwh) + '</td><td>¥ ' + formatNumber(row.total_bill) + '</td><td>' + formatNumber(row.household_count, 0) + '</td><td>' + formatNumber(row.average_kwh_household) + '</td><td>¥ ' + formatNumber(row.average_bill_household) + '</td><td><strong class="score-value">' + formatNumber(row.priority_score, 3) + '</strong></td></tr>').join('') : '<tr><td class="empty-cell" colspan="10">当前条件下没有推荐小区。</td></tr>';
}
async function recommend() {
  if (!state.role) return;
  const count = Number($('demand-count').value) || 0;
  const minimumBill = Number($('demand-bill').value) || 0;
  if (count < 0 || !Number.isInteger(count)) return message('recommend-message', '扩容小区数量须为非负整数。', 'error');
  if (minimumBill < 0 || !Number.isFinite(minimumBill)) return message('recommend-message', '最低电费须为非负数。', 'error');
  $('recommend-btn').disabled = true;
  message('recommend-message', '正在计算推荐名单…', 'info');
  try {
    const rows = await api('POST', API.recommend, {
      body: { target_community_count: count, min_total_bill: minimumBill }, auth: true
    });
    renderRecommendations(rows);
    message('recommend-message', '推荐名单已生成，共 ' + rows.length + ' 个小区。', rows.length ? 'success' : 'info');
  } catch (error) {
    message('recommend-message', error.message, 'error');
  } finally {
    $('recommend-btn').disabled = false;
  }
}
function bindEvents() {
  $('login-form').addEventListener('submit', login);
  document.querySelectorAll('[data-page]').forEach(button => button.addEventListener('click', () => go(button.dataset.page)));
  document.querySelectorAll('[data-open]').forEach(button => button.addEventListener('click', () => go(button.dataset.open)));
  $('top-login').addEventListener('click', logout);
  $('side-login').addEventListener('click', logout);
  $('overview-load').addEventListener('click', loadOverview);
  $('overview-query-community').addEventListener('keydown', event => { if (event.key === 'Enter') loadOverview(); });
  $('query-btn').addEventListener('click', () => queryRecords(1));
  $('prev').addEventListener('click', () => { if (state.page > 1) queryRecords(state.page - 1); });
  $('next').addEventListener('click', () => { if (state.count === PAGE_SIZE) queryRecords(state.page + 1); });
  $('save-entry').addEventListener('click', saveRecord);
  $('cancel-edit').addEventListener('click', clearEntry);
  $('report-btn').addEventListener('click', report);
  $('sort-btn').addEventListener('click', sortData);
  $('recommend-btn').addEventListener('click', recommend);
  document.querySelectorAll('[data-chart-type]').forEach(button => button.addEventListener('click', () => {
    state.chartType = button.dataset.chartType;
    renderReportChart();
  }));
}

bindEvents();
showWorkspace();
checkHealth();
