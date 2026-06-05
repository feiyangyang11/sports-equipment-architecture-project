import { DEFAULT_PAGE_SIZE } from "./config.js";
import { reservationApi } from "./api.js";
import { requireRole } from "./guard.js";
import { mountShell } from "./shell.js";
import { bindPagination, escapeHtml, formatDateTime, renderPagination, renderStatusPill } from "./utils.js";
import { bootstrapModal, safeErrorMessage, setFeedback } from "./ui.js";

const currentUser = requireRole("STUDENT");

const state = {
  pageNo: 1,
  pageSize: DEFAULT_PAGE_SIZE,
  status: "",
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

function canCancel(reservation) {
  return reservation && ["PENDING", "APPROVED"].includes(reservation.status);
}

function renderReservationTable(items) {
  if (!items.length) {
    return `<div class="empty-state">当前筛选条件下没有预约记录，可以先去器材页创建一条新的预约。</div>`;
  }

  return `
    <table class="data-table">
      <thead>
        <tr>
          <th>预约单号</th>
          <th>器材</th>
          <th>时间段</th>
          <th>数量</th>
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
              <strong>${escapeHtml(item.equipmentName)}</strong>
              <div class="text-secondary small">${escapeHtml(item.equipmentCode)}</div>
            </td>
            <td>
              <div>${formatDateTime(item.reservationStartAt)}</div>
              <div class="text-secondary small">至 ${formatDateTime(item.reservationEndAt)}</div>
            </td>
            <td>${item.quantity}</td>
            <td>${renderStatusPill(item.status)}</td>
            <td>
              <div class="button-row">
                <button class="btn btn--ghost" data-action="detail" data-id="${item.id}">详情</button>
                ${canCancel(item) ? `<button class="btn btn--danger" data-action="cancel" data-id="${item.id}">取消</button>` : ""}
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

  const timelineItems = [
    { title: "创建时间", meta: formatDateTime(item.createdAt) },
    item.reviewedAt ? { title: "审核时间", meta: formatDateTime(item.reviewedAt) } : null,
    item.cancelReason ? { title: "取消原因", meta: item.cancelReason } : null,
  ].filter(Boolean);

  return `
    <section class="panel detail-card">
      <div class="panel__header">
        <div>
          <h3 class="panel__title">${escapeHtml(item.equipmentName)}</h3>
          <p class="panel__subtitle">${escapeHtml(item.reservationNo)} · ${escapeHtml(item.equipmentCode)}</p>
        </div>
        ${renderStatusPill(item.status)}
      </div>
      <div class="detail-list">
        <div class="detail-item">
          <p class="detail-item__label">预约时间段</p>
          <p class="detail-item__value">${formatDateTime(item.reservationStartAt)} 至 ${formatDateTime(item.reservationEndAt)}</p>
        </div>
        <div class="detail-item">
          <p class="detail-item__label">数量</p>
          <p class="detail-item__value">${item.quantity}</p>
        </div>
        <div class="detail-item">
          <p class="detail-item__label">申请说明</p>
          <p class="detail-item__value">${escapeHtml(item.requestNote || "无")}</p>
        </div>
        <div class="detail-item">
          <p class="detail-item__label">审核意见</p>
          <p class="detail-item__value">${escapeHtml(item.reviewNote || "暂无")}</p>
        </div>
      </div>
      <div class="timeline" style="margin-top: 18px;">
        ${timelineItems.map((timeline) => `
          <article class="timeline-item">
            <p class="timeline-item__title">${escapeHtml(timeline.title)}</p>
            <p class="timeline-item__meta">${escapeHtml(timeline.meta)}</p>
          </article>
        `).join("")}
      </div>
      ${canCancel(item) ? `
        <div class="button-row" style="margin-top: 18px;">
          <button class="btn btn--danger" data-action="cancel" data-id="${item.id}">取消当前预约</button>
        </div>
      ` : ""}
    </section>
  `;
}

function renderCancelModal(item) {
  return `
    <div class="modal fade" id="cancelReservationModal" tabindex="-1" aria-hidden="true">
      <div class="modal-dialog modal-dialog-centered">
        <div class="modal-content" style="border-radius: 22px; border: 1px solid rgba(16, 32, 51, 0.1);">
          <div class="modal-header" style="border-bottom: 1px solid rgba(16, 32, 51, 0.08);">
            <div>
              <h5 class="modal-title">取消预约</h5>
              <div class="text-secondary small">${escapeHtml(item?.reservationNo || "未选择预约")}</div>
            </div>
            <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="关闭"></button>
          </div>
          <form id="cancelReservationForm">
            <div class="modal-body">
              <div class="feedback" id="cancelReservationFeedback"></div>
              <input type="hidden" name="reservationId" value="${item?.id || ""}" />
              <div class="field">
                <label for="cancelReason">取消原因</label>
                <textarea id="cancelReason" name="cancelReason" placeholder="例如：时间冲突、计划变更、器材不再需要。"></textarea>
              </div>
            </div>
            <div class="modal-footer" style="border-top: 1px solid rgba(16, 32, 51, 0.08);">
              <button type="button" class="btn btn--ghost" data-bs-dismiss="modal">先保留</button>
              <button type="submit" class="btn btn--danger">确认取消</button>
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
  const completedCount = state.pageResult.items.filter((item) => item.status === "COMPLETED").length;

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
        <p class="stat-card__label">已完成</p>
        <p class="stat-card__value">${completedCount}</p>
      </article>
    </section>
  `;

  const main = `
    <section class="panel">
      <div class="panel__header">
        <div>
          <h3 class="panel__title">我的预约记录</h3>
          <p class="panel__subtitle">查看个人预约记录、当前状态，以及可取消的预约申请。</p>
        </div>
      </div>
      <div class="feedback ${state.pageFeedback.message ? `feedback--${state.pageFeedback.type} is-visible` : ""}" id="reservationPageFeedback">${escapeHtml(state.pageFeedback.message)}</div>
      <div class="toolbar toolbar--compact">
        <div class="field">
          <label for="statusFilter">状态筛选</label>
          <select id="statusFilter">
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
    ${renderCancelModal(selected)}
  `;

  mountShell({
    user: currentUser,
    role: "STUDENT",
    navKey: "student-reservations",
    eyebrow: "Student Reservation Board",
    title: "我的预约",
    lead: "查看预约申请的审核进度、预约时间和器材信息。",
    stats,
    main,
    aside: renderReservationDetail(selected),
    topBadge: "学生端",
  });

  bindEvents();
}

function openCancelModal(id) {
  state.selectedReservationId = Number(id);
  const selected = getSelectedReservation();
  const modalElement = document.getElementById("cancelReservationModal");
  modalElement.querySelector("input[name='reservationId']").value = String(selected?.id || "");
  bootstrapModal("cancelReservationModal")?.show();
}

function bindEvents() {
  document.getElementById("statusFilter")?.addEventListener("change", (event) => {
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

  document.querySelectorAll("[data-action='cancel']").forEach((button) => {
    button.addEventListener("click", () => openCancelModal(button.dataset.id));
  });

  const paginationRoot = document.querySelector(".pagination");
  if (paginationRoot) {
    bindPagination(paginationRoot, (nextPage) => {
      state.pageNo = nextPage;
      loadPage();
    });
  }

  document.getElementById("cancelReservationForm")?.addEventListener("submit", async (event) => {
    event.preventDefault();
    const formData = new FormData(event.currentTarget);
    const feedback = document.getElementById("cancelReservationFeedback");
    setFeedback(feedback, "");

    try {
      await reservationApi.cancelMine(Number(formData.get("reservationId")), {
        cancelReason: String(formData.get("cancelReason") || "").trim(),
      });
      bootstrapModal("cancelReservationModal")?.hide();
      state.pageFeedback = {
        message: "预约已取消，列表和详情会同步反映这个变化。",
        type: "success",
      };
      loadPage();
    } catch (error) {
      setFeedback(feedback, safeErrorMessage(error, "取消预约失败。"));
    }
  });
}

async function loadPage() {
  try {
    state.pageResult = await reservationApi.getMine({
      pageNo: state.pageNo,
      pageSize: state.pageSize,
      status: state.status || undefined,
    });
    if (!state.selectedReservationId || !state.pageResult.items.some((item) => item.id === state.selectedReservationId)) {
      state.selectedReservationId = state.pageResult.items[0]?.id || null;
    }
    render();
  } catch (error) {
    state.pageFeedback = { message: safeErrorMessage(error, "预约列表加载失败。"), type: "error" };
    state.pageResult = { pageNo: 1, pageSize: state.pageSize, total: 0, items: [] };
    render();
  }
}

loadPage();
