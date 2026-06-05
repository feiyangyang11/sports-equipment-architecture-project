import { DEFAULT_PAGE_SIZE } from "./config.js";
import { reservationApi } from "./api.js";
import { requireRole } from "./guard.js";
import { mountShell } from "./shell.js";
import { bindPagination, escapeHtml, formatDateTime, renderPagination, renderStatusPill } from "./utils.js";
import { bootstrapModal, safeErrorMessage, setFeedback } from "./ui.js";

const currentUser = requireRole("ADMIN");

const state = {
  pageNo: 1,
  pageSize: DEFAULT_PAGE_SIZE,
  status: "",
  reviewAction: "approve",
  pageResult: { pageNo: 1, pageSize: DEFAULT_PAGE_SIZE, total: 0, items: [] },
  selectedReservationId: null,
  pageFeedback: { message: "", type: "success" },
};

function getSelectedReservation() {
  return (
    state.pageResult.items.find((item) => item.id === state.selectedReservationId) ||
    state.pageResult.items[0] ||
    null
  );
}

function isReviewable(item) {
  return item?.status === "PENDING";
}

function renderReservationTable(items) {
  if (!items.length) {
    return `<div class="empty-state">当前筛选条件下没有预约记录，请切换筛选条件后重试。</div>`;
  }

  return `
    <table class="data-table">
      <thead>
        <tr>
          <th>预约单号</th>
          <th>申请人</th>
          <th>器材</th>
          <th>时间段</th>
          <th>状态</th>
          <th>操作</th>
        </tr>
      </thead>
      <tbody>
        ${items.map((item) => `
          <tr>
            <td>
              <strong>${escapeHtml(item.reservationNo)}</strong>
              <div class="text-secondary small">${formatDateTime(item.createdAt)}</div>
            </td>
            <td>
              <strong>${escapeHtml(item.studentRealName)}</strong>
              <div class="text-secondary small">${escapeHtml(item.studentUsername)}</div>
            </td>
            <td>
              <strong>${escapeHtml(item.equipmentName)}</strong>
              <div class="text-secondary small">${escapeHtml(item.equipmentCode)}</div>
            </td>
            <td>
              <div>${formatDateTime(item.reservationStartAt)}</div>
              <div class="text-secondary small">至 ${formatDateTime(item.reservationEndAt)}</div>
            </td>
            <td>${renderStatusPill(item.status)}</td>
            <td>
              <div class="button-row">
                <button class="btn btn--ghost" data-action="detail" data-id="${item.id}">详情</button>
                ${isReviewable(item) ? `
                  <button class="btn btn--success" data-action="approve" data-id="${item.id}">通过</button>
                  <button class="btn btn--danger" data-action="reject" data-id="${item.id}">拒绝</button>
                ` : ""}
              </div>
            </td>
          </tr>
        `).join("")}
      </tbody>
    </table>
  `;
}

function renderReservationDetail(item) {
  if (!item) {
    return `<section class="panel"><div class="empty-state">当前页暂无可查看的预约详情。</div></section>`;
  }

  return `
    <section class="panel detail-card">
      <div class="panel__header">
        <div>
          <h3 class="panel__title">${escapeHtml(item.studentRealName)}</h3>
          <p class="panel__subtitle">${escapeHtml(item.reservationNo)} · ${escapeHtml(item.studentUsername)}</p>
        </div>
        ${renderStatusPill(item.status)}
      </div>
      <div class="detail-list">
        <div class="detail-item">
          <p class="detail-item__label">预约器材</p>
          <p class="detail-item__value">${escapeHtml(item.equipmentName)}（${escapeHtml(item.equipmentCode)}）</p>
        </div>
        <div class="detail-item">
          <p class="detail-item__label">预约时间段</p>
          <p class="detail-item__value">${formatDateTime(item.reservationStartAt)} 至 ${formatDateTime(item.reservationEndAt)}</p>
        </div>
        <div class="detail-item">
          <p class="detail-item__label">申请说明</p>
          <p class="detail-item__value">${escapeHtml(item.requestNote || "无")}</p>
        </div>
        <div class="detail-item">
          <p class="detail-item__label">审核意见</p>
          <p class="detail-item__value">${escapeHtml(item.reviewNote || "尚未填写")}</p>
        </div>
      </div>
      ${isReviewable(item) ? `
        <div class="button-row" style="margin-top: 18px;">
          <button class="btn btn--success" data-action="approve" data-id="${item.id}">审核通过</button>
          <button class="btn btn--danger" data-action="reject" data-id="${item.id}">审核拒绝</button>
        </div>
      ` : ""}
    </section>
  `;
}

function renderReviewModal(item) {
  return `
    <div class="modal fade" id="reviewReservationModal" tabindex="-1" aria-hidden="true">
      <div class="modal-dialog modal-dialog-centered">
        <div class="modal-content" style="border-radius: 22px; border: 1px solid rgba(16, 32, 51, 0.1);">
          <div class="modal-header" style="border-bottom: 1px solid rgba(16, 32, 51, 0.08);">
            <div>
              <h5 class="modal-title" id="reviewModalTitle">审核预约</h5>
              <div class="text-secondary small">${escapeHtml(item?.reservationNo || "未选择预约")}</div>
            </div>
            <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="关闭"></button>
          </div>
          <form id="reviewReservationForm">
            <div class="modal-body">
              <div class="feedback" id="reviewReservationFeedback"></div>
              <input type="hidden" name="reservationId" value="${item?.id || ""}" />
              <input type="hidden" name="reviewAction" value="${state.reviewAction}" />
              <div class="field">
                <label for="reviewNote">审核意见</label>
                <textarea id="reviewNote" name="reviewNote" placeholder="给出简短、明确的审核说明。"></textarea>
              </div>
            </div>
            <div class="modal-footer" style="border-top: 1px solid rgba(16, 32, 51, 0.08);">
              <button type="button" class="btn btn--ghost" data-bs-dismiss="modal">返回</button>
              <button type="submit" class="btn ${state.reviewAction === "approve" ? "btn--success" : "btn--danger"}" id="reviewSubmitButton">
                ${state.reviewAction === "approve" ? "确认通过" : "确认拒绝"}
              </button>
            </div>
          </form>
        </div>
      </div>
    </div>
  `;
}

function render() {
  const selected = getSelectedReservation();
  const pendingCount = state.pageResult.items.filter((item) => item.status === "PENDING").length;
  const approvedCount = state.pageResult.items.filter((item) => item.status === "APPROVED").length;
  const rejectedCount = state.pageResult.items.filter((item) => item.status === "REJECTED").length;

  const stats = `
    <section class="summary-grid fade-up">
      <article class="stat-card">
        <p class="stat-card__label">当前页预约数</p>
        <p class="stat-card__value">${state.pageResult.items.length}</p>
      </article>
      <article class="stat-card">
        <p class="stat-card__label">待审核</p>
        <p class="stat-card__value">${pendingCount}</p>
      </article>
      <article class="stat-card">
        <p class="stat-card__label">已通过</p>
        <p class="stat-card__value">${approvedCount}</p>
      </article>
      <article class="stat-card">
        <p class="stat-card__label">已拒绝</p>
        <p class="stat-card__value">${rejectedCount}</p>
      </article>
    </section>
  `;

  const main = `
    <section class="panel">
      <div class="panel__header">
        <div>
          <h3 class="panel__title">预约审核队列</h3>
          <p class="panel__subtitle">查看预约申请人、器材信息和预约时间，并完成审核处理。</p>
        </div>
      </div>
      <div class="feedback ${state.pageFeedback.message ? `feedback--${state.pageFeedback.type} is-visible` : ""}">${escapeHtml(state.pageFeedback.message)}</div>
      <div class="toolbar toolbar--compact">
        <div class="field">
          <label for="adminReservationStatusFilter">状态筛选</label>
          <select id="adminReservationStatusFilter">
            <option value="">全部状态</option>
            ${["PENDING", "APPROVED", "REJECTED", "CANCELED", "BORROWED", "COMPLETED"].map((status) => `
              <option value="${status}" ${state.status === status ? "selected" : ""}>${status}</option>
            `).join("")}
          </select>
        </div>
      </div>
      ${renderReservationTable(state.pageResult.items)}
      ${renderPagination(state.pageResult)}
    </section>
    ${renderReviewModal(selected)}
  `;

  mountShell({
    user: currentUser,
    role: "ADMIN",
    navKey: "admin-reservations",
    eyebrow: "Admin Reservation Review",
    title: "预约审核",
    lead: "查看预约申请详情，并完成通过或拒绝操作。",
    stats,
    main,
    aside: renderReservationDetail(selected),
    topBadge: "管理员端",
  });

  bindEvents();
}

function openReviewModal(id, action) {
  state.selectedReservationId = Number(id);
  state.reviewAction = action;
  const selected = getSelectedReservation();
  const modalElement = document.getElementById("reviewReservationModal");
  modalElement.querySelector("input[name='reservationId']").value = String(selected?.id || "");
  modalElement.querySelector("input[name='reviewAction']").value = action;
  modalElement.querySelector("#reviewModalTitle").textContent = action === "approve" ? "审核通过预约" : "审核拒绝预约";
  modalElement.querySelector("#reviewSubmitButton").textContent = action === "approve" ? "确认通过" : "确认拒绝";
  modalElement.querySelector("#reviewSubmitButton").className = `btn ${action === "approve" ? "btn--success" : "btn--danger"}`;
  bootstrapModal("reviewReservationModal")?.show();
}

function bindEvents() {
  document.getElementById("adminReservationStatusFilter")?.addEventListener("change", (event) => {
    state.status = event.target.value;
    state.pageNo = 1;
    loadPage();
  });

  document.querySelectorAll("[data-action='detail']").forEach((button) => {
    button.addEventListener("click", () => {
      state.selectedReservationId = Number(button.dataset.id);
      render();
    });
  });

  document.querySelectorAll("[data-action='approve'], [data-action='reject']").forEach((button) => {
    button.addEventListener("click", () => {
      openReviewModal(button.dataset.id, button.dataset.action);
    });
  });

  const paginationRoot = document.querySelector(".pagination");
  if (paginationRoot) {
    bindPagination(paginationRoot, (nextPage) => {
      state.pageNo = nextPage;
      loadPage();
    });
  }

  document.getElementById("reviewReservationForm")?.addEventListener("submit", async (event) => {
    event.preventDefault();
    const formData = new FormData(event.currentTarget);
    const feedback = document.getElementById("reviewReservationFeedback");
    setFeedback(feedback, "");
    const reservationId = Number(formData.get("reservationId"));
    const action = String(formData.get("reviewAction"));
    const payload = {
      reviewNote: String(formData.get("reviewNote") || "").trim(),
    };

    try {
      if (action === "approve") {
        await reservationApi.approve(reservationId, payload);
      } else {
        await reservationApi.reject(reservationId, payload);
      }
      bootstrapModal("reviewReservationModal")?.hide();
      state.pageFeedback = {
        message: action === "approve" ? "预约已审核通过，可以进入借出办理阶段。" : "预约已拒绝，状态已写回列表。",
        type: "success",
      };
      loadPage();
    } catch (error) {
      setFeedback(feedback, safeErrorMessage(error, "审核操作失败。"));
    }
  });
}

async function loadPage() {
  try {
    state.pageResult = await reservationApi.getAdminPage({
      pageNo: state.pageNo,
      pageSize: state.pageSize,
      status: state.status || undefined,
    });
    if (!state.selectedReservationId || !state.pageResult.items.some((item) => item.id === state.selectedReservationId)) {
      state.selectedReservationId = state.pageResult.items[0]?.id || null;
    }
    render();
  } catch (error) {
    state.pageFeedback = { message: safeErrorMessage(error, "管理员预约列表加载失败。"), type: "error" };
    state.pageResult = { pageNo: 1, pageSize: state.pageSize, total: 0, items: [] };
    render();
  }
}

loadPage();
