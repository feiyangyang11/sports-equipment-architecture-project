import { DEFAULT_PAGE_SIZE } from "./config.js";
import { borrowApi } from "./api.js";
import { requireRole } from "./guard.js";
import { mountShell } from "./shell.js";
import { bindPagination, escapeHtml, formatDateTime, renderPagination, renderStatusPill } from "./utils.js";
import { safeErrorMessage } from "./ui.js";

const currentUser = requireRole("STUDENT");

const state = {
  pageNo: 1,
  pageSize: DEFAULT_PAGE_SIZE,
  status: "",
  pageResult: { pageNo: 1, pageSize: DEFAULT_PAGE_SIZE, total: 0, items: [] },
  selectedBorrowId: null,
  pageFeedback: { message: "", type: "success" },
};

function getSelectedBorrow() {
  return (
    state.pageResult.items.find((item) => item.id === state.selectedBorrowId) ||
    state.pageResult.items[0] ||
    null
  );
}

function renderBorrowTable(items) {
  if (!items.length) {
    return `<div class="empty-state">当前筛选条件下没有借用记录，请切换筛选条件或稍后再查看。</div>`;
  }

  return `
    <table class="data-table">
      <thead>
        <tr>
          <th>借用单号</th>
          <th>器材</th>
          <th>借出 / 截止</th>
          <th>数量</th>
          <th>状态</th>
          <th>操作</th>
        </tr>
      </thead>
      <tbody>
        ${items.map((item) => `
          <tr>
            <td>
              <strong>${escapeHtml(item.borrowNo)}</strong>
              <div class="text-secondary small">${escapeHtml(item.reservationNo)}</div>
            </td>
            <td>
              <strong>${escapeHtml(item.equipmentName)}</strong>
              <div class="text-secondary small">${escapeHtml(item.equipmentCode)}</div>
            </td>
            <td>
              <div>${formatDateTime(item.borrowedAt)}</div>
              <div class="text-secondary small">截至 ${formatDateTime(item.dueAt)}</div>
            </td>
            <td>${item.quantity}</td>
            <td>${renderStatusPill(item.status)}</td>
            <td>
              <button class="btn btn--ghost" data-action="detail" data-id="${item.id}">详情</button>
            </td>
          </tr>
        `).join("")}
      </tbody>
    </table>
  `;
}

function renderBorrowDetail(item) {
  if (!item) {
    return `<section class="panel"><div class="empty-state">当前页暂无可查看的借用详情。</div></section>`;
  }

  return `
    <section class="panel detail-card">
      <div class="panel__header">
        <div>
          <h3 class="panel__title">${escapeHtml(item.equipmentName)}</h3>
          <p class="panel__subtitle">${escapeHtml(item.borrowNo)} · ${escapeHtml(item.reservationNo)}</p>
        </div>
        ${renderStatusPill(item.status)}
      </div>
      <div class="detail-list">
        <div class="detail-item">
          <p class="detail-item__label">借出时间</p>
          <p class="detail-item__value">${formatDateTime(item.borrowedAt)}</p>
        </div>
        <div class="detail-item">
          <p class="detail-item__label">应还时间</p>
          <p class="detail-item__value">${formatDateTime(item.dueAt)}</p>
        </div>
        <div class="detail-item">
          <p class="detail-item__label">实际归还时间</p>
          <p class="detail-item__value">${formatDateTime(item.returnedAt)}</p>
        </div>
        <div class="detail-item">
          <p class="detail-item__label">归还备注</p>
          <p class="detail-item__value">${escapeHtml(item.returnNote || "暂无")}</p>
        </div>
      </div>
    </section>
  `;
}

function render() {
  const selected = getSelectedBorrow();
  const borrowingCount = state.pageResult.items.filter((item) => item.status === "BORROWING").length;
  const returnedCount = state.pageResult.items.filter((item) => item.status === "RETURNED").length;
  const overdueCount = state.pageResult.items.filter((item) => item.status === "OVERDUE").length;

  const stats = `
    <section class="summary-grid fade-up">
      <article class="stat-card">
        <p class="stat-card__label">当前页借用数</p>
        <p class="stat-card__value">${state.pageResult.items.length}</p>
      </article>
      <article class="stat-card">
        <p class="stat-card__label">借出中</p>
        <p class="stat-card__value">${borrowingCount}</p>
      </article>
      <article class="stat-card">
        <p class="stat-card__label">已归还</p>
        <p class="stat-card__value">${returnedCount}</p>
      </article>
      <article class="stat-card">
        <p class="stat-card__label">逾期</p>
        <p class="stat-card__value">${overdueCount}</p>
      </article>
    </section>
  `;

  const main = `
    <section class="panel">
      <div class="panel__header">
        <div>
          <h3 class="panel__title">我的借用记录</h3>
          <p class="panel__subtitle">查看借用状态、归还情况和应还时间，便于及时安排归还。</p>
        </div>
      </div>
      <div class="feedback ${state.pageFeedback.message ? `feedback--${state.pageFeedback.type} is-visible` : ""}">${escapeHtml(state.pageFeedback.message)}</div>
      <div class="toolbar toolbar--compact">
        <div class="field">
          <label for="borrowStatusFilter">状态筛选</label>
          <select id="borrowStatusFilter">
            <option value="">全部状态</option>
            ${["BORROWING", "RETURNED", "OVERDUE", "CLOSED"].map((status) => `
              <option value="${status}" ${state.status === status ? "selected" : ""}>${status}</option>
            `).join("")}
          </select>
        </div>
      </div>
      ${renderBorrowTable(state.pageResult.items)}
      ${renderPagination(state.pageResult)}
    </section>
  `;

  mountShell({
    user: currentUser,
    role: "STUDENT",
    navKey: "student-borrows",
    eyebrow: "Student Borrow Tracker",
    title: "我的借用",
    lead: "查看个人借用记录、借出期限和归还状态。",
    stats,
    main,
    aside: renderBorrowDetail(selected),
    topBadge: "学生端",
  });

  bindEvents();
}

function bindEvents() {
  document.getElementById("borrowStatusFilter")?.addEventListener("change", (event) => {
    state.status = event.target.value;
    state.pageNo = 1;
    loadPage();
  });

  document.querySelectorAll("[data-action='detail']").forEach((button) => {
    button.addEventListener("click", () => {
      state.selectedBorrowId = Number(button.dataset.id);
      render();
    });
  });

  const paginationRoot = document.querySelector(".pagination");
  if (paginationRoot) {
    bindPagination(paginationRoot, (nextPage) => {
      state.pageNo = nextPage;
      loadPage();
    });
  }
}

async function loadPage() {
  try {
    state.pageResult = await borrowApi.getMine({
      pageNo: state.pageNo,
      pageSize: state.pageSize,
      status: state.status || undefined,
    });
    if (!state.selectedBorrowId || !state.pageResult.items.some((item) => item.id === state.selectedBorrowId)) {
      state.selectedBorrowId = state.pageResult.items[0]?.id || null;
    }
    render();
  } catch (error) {
    state.pageFeedback = { message: safeErrorMessage(error, "借用列表加载失败。"), type: "error" };
    state.pageResult = { pageNo: 1, pageSize: state.pageSize, total: 0, items: [] };
    render();
  }
}

loadPage();
