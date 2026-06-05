import { DEFAULT_PAGE_SIZE } from "./config.js";
import { borrowApi, reservationApi } from "./api.js";
import { requireRole } from "./guard.js";
import { mountShell } from "./shell.js";
import { bindPagination, escapeHtml, formatDateTime, renderPagination, renderStatusPill } from "./utils.js";
import { bootstrapModal, safeErrorMessage, setFeedback } from "./ui.js";

const currentUser = requireRole("ADMIN");

const state = {
  borrowPageNo: 1,
  pageSize: DEFAULT_PAGE_SIZE,
  status: "",
  approvedReservations: [],
  borrowPage: { pageNo: 1, pageSize: DEFAULT_PAGE_SIZE, total: 0, items: [] },
  selectedBorrowId: null,
  createBorrowReservationId: null,
  pageFeedback: { message: "", type: "success" },
};

function getSelectedBorrow() {
  return (
    state.borrowPage.items.find((item) => item.id === state.selectedBorrowId) ||
    state.borrowPage.items[0] ||
    null
  );
}

function renderApprovedReservations() {
  if (!state.approvedReservations.length) {
    return `<div class="empty-state">当前没有待办理借出的预约记录，请稍后再查看。</div>`;
  }

  return `
    <div class="stack">
      ${state.approvedReservations.map((item) => `
        <article class="list-card">
          <div class="list-card__title">${escapeHtml(item.equipmentName)}</div>
          <p class="list-card__meta">${escapeHtml(item.studentRealName)} · ${escapeHtml(item.studentUsername)}</p>
          <div class="chips" style="margin-top: 12px;">
            <span class="chip">${escapeHtml(item.reservationNo)}</span>
            <span class="chip chip--muted">数量 ${item.quantity}</span>
            ${renderStatusPill(item.status)}
          </div>
          <div class="list-card__footer">
            <div class="text-secondary small">${formatDateTime(item.reservationStartAt)} 至 ${formatDateTime(item.reservationEndAt)}</div>
            <button class="btn btn--primary" data-action="borrow" data-id="${item.id}">办理借出</button>
          </div>
        </article>
      `).join("")}
    </div>
  `;
}

function renderBorrowTable(items) {
  if (!items.length) {
    return `<div class="empty-state">当前筛选条件下没有借用记录，请切换筛选条件后重试。</div>`;
  }

  return `
    <table class="data-table">
      <thead>
        <tr>
          <th>借用单号</th>
          <th>学生</th>
          <th>器材</th>
          <th>借出 / 截止</th>
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
              <strong>${escapeHtml(item.studentRealName)}</strong>
              <div class="text-secondary small">${escapeHtml(item.studentUsername)}</div>
            </td>
            <td>
              <strong>${escapeHtml(item.equipmentName)}</strong>
              <div class="text-secondary small">${escapeHtml(item.equipmentCode)}</div>
            </td>
            <td>
              <div>${formatDateTime(item.borrowedAt)}</div>
              <div class="text-secondary small">至 ${formatDateTime(item.dueAt)}</div>
            </td>
            <td>${renderStatusPill(item.status)}</td>
            <td>
              <div class="button-row">
                <button class="btn btn--ghost" data-action="detail" data-id="${item.id}">详情</button>
                ${item.status === "BORROWING" ? `<button class="btn btn--success" data-action="return" data-id="${item.id}">办理归还</button>` : ""}
              </div>
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
          <h3 class="panel__title">${escapeHtml(item.studentRealName)}</h3>
          <p class="panel__subtitle">${escapeHtml(item.borrowNo)} · ${escapeHtml(item.studentUsername)}</p>
        </div>
        ${renderStatusPill(item.status)}
      </div>
      <div class="detail-list">
        <div class="detail-item">
          <p class="detail-item__label">器材信息</p>
          <p class="detail-item__value">${escapeHtml(item.equipmentName)}（${escapeHtml(item.equipmentCode)}）</p>
        </div>
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
      ${item.status === "BORROWING" ? `
        <div class="button-row" style="margin-top: 18px;">
          <button class="btn btn--success" data-action="return" data-id="${item.id}">办理归还</button>
        </div>
      ` : ""}
    </section>
  `;
}

function renderBorrowModal() {
  const selected = state.approvedReservations.find((item) => item.id === state.createBorrowReservationId);
  return `
    <div class="modal fade" id="createBorrowModal" tabindex="-1" aria-hidden="true">
      <div class="modal-dialog modal-dialog-centered">
        <div class="modal-content" style="border-radius: 22px; border: 1px solid rgba(16, 32, 51, 0.1);">
          <div class="modal-header" style="border-bottom: 1px solid rgba(16, 32, 51, 0.08);">
            <div>
              <h5 class="modal-title">办理借出</h5>
              <div class="text-secondary small">${escapeHtml(selected?.reservationNo || "未选择预约")}</div>
            </div>
            <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="关闭"></button>
          </div>
          <form id="createBorrowForm">
            <div class="modal-body">
              <div class="feedback" id="createBorrowFeedback"></div>
              <input type="hidden" name="reservationId" value="${selected?.id || ""}" />
              <div class="field">
                <label for="borrowedAt">借出时间</label>
                <input id="borrowedAt" name="borrowedAt" type="text" placeholder="2026-06-04 15:00:00" />
              </div>
              <div class="field" style="margin-top: 12px;">
                <label for="dueAt">应还时间</label>
                <input id="dueAt" name="dueAt" type="text" placeholder="2026-06-06 18:00:00" />
              </div>
            </div>
            <div class="modal-footer" style="border-top: 1px solid rgba(16, 32, 51, 0.08);">
              <button type="button" class="btn btn--ghost" data-bs-dismiss="modal">取消</button>
              <button type="submit" class="btn btn--primary">确认借出</button>
            </div>
          </form>
        </div>
      </div>
    </div>
  `;
}

function renderReturnModal() {
  const selected = getSelectedBorrow();
  return `
    <div class="modal fade" id="returnBorrowModal" tabindex="-1" aria-hidden="true">
      <div class="modal-dialog modal-dialog-centered">
        <div class="modal-content" style="border-radius: 22px; border: 1px solid rgba(16, 32, 51, 0.1);">
          <div class="modal-header" style="border-bottom: 1px solid rgba(16, 32, 51, 0.08);">
            <div>
              <h5 class="modal-title">办理归还</h5>
              <div class="text-secondary small">${escapeHtml(selected?.borrowNo || "未选择借用记录")}</div>
            </div>
            <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="关闭"></button>
          </div>
          <form id="returnBorrowForm">
            <div class="modal-body">
              <div class="feedback" id="returnBorrowFeedback"></div>
              <input type="hidden" name="borrowRecordId" value="${selected?.id || ""}" />
              <div class="field">
                <label for="returnedAt">归还时间</label>
                <input id="returnedAt" name="returnedAt" type="text" placeholder="2026-06-05 10:30:00" />
              </div>
              <div class="field" style="margin-top: 12px;">
                <label for="returnNote">归还备注</label>
                <textarea id="returnNote" name="returnNote" placeholder="例如：器材完好归还。"></textarea>
              </div>
            </div>
            <div class="modal-footer" style="border-top: 1px solid rgba(16, 32, 51, 0.08);">
              <button type="button" class="btn btn--ghost" data-bs-dismiss="modal">取消</button>
              <button type="submit" class="btn btn--success">确认归还</button>
            </div>
          </form>
        </div>
      </div>
    </div>
  `;
}

function render() {
  const selected = getSelectedBorrow();
  const borrowingCount = state.borrowPage.items.filter((item) => item.status === "BORROWING").length;
  const returnedCount = state.borrowPage.items.filter((item) => item.status === "RETURNED").length;

  const stats = `
    <section class="summary-grid fade-up">
      <article class="stat-card">
        <p class="stat-card__label">待借出预约</p>
        <p class="stat-card__value">${state.approvedReservations.length}</p>
      </article>
      <article class="stat-card">
        <p class="stat-card__label">当前页借用数</p>
        <p class="stat-card__value">${state.borrowPage.items.length}</p>
      </article>
      <article class="stat-card">
        <p class="stat-card__label">借出中</p>
        <p class="stat-card__value">${borrowingCount}</p>
      </article>
      <article class="stat-card">
        <p class="stat-card__label">已归还</p>
        <p class="stat-card__value">${returnedCount}</p>
      </article>
    </section>
  `;

  const main = `
    <section class="panel">
      <div class="panel__header">
        <div>
          <h3 class="panel__title">待办理借出</h3>
          <p class="panel__subtitle">这里展示已审核通过、待办理借出的预约记录，可直接完成借出登记。</p>
        </div>
      </div>
      ${renderApprovedReservations()}
    </section>
    <section class="panel">
      <div class="panel__header">
        <div>
          <h3 class="panel__title">借还记录</h3>
          <p class="panel__subtitle">查看借用记录、跟踪借出状态，并及时办理归还。</p>
        </div>
      </div>
      <div class="feedback ${state.pageFeedback.message ? `feedback--${state.pageFeedback.type} is-visible` : ""}">${escapeHtml(state.pageFeedback.message)}</div>
      <div class="toolbar toolbar--compact">
        <div class="field">
          <label for="borrowStatusFilterAdmin">状态筛选</label>
          <select id="borrowStatusFilterAdmin">
            <option value="">全部状态</option>
            ${["BORROWING", "RETURNED", "OVERDUE", "CLOSED"].map((status) => `
              <option value="${status}" ${state.status === status ? "selected" : ""}>${status}</option>
            `).join("")}
          </select>
        </div>
      </div>
      ${renderBorrowTable(state.borrowPage.items)}
      ${renderPagination(state.borrowPage)}
    </section>
    ${renderBorrowModal()}
    ${renderReturnModal()}
  `;

  mountShell({
    user: currentUser,
    role: "ADMIN",
    navKey: "admin-borrows",
    eyebrow: "Admin Borrow Console",
    title: "借还管理",
    lead: "处理已审核预约的借出登记，并维护器材归还记录。",
    stats,
    main,
    aside: renderBorrowDetail(selected),
    topBadge: "管理员端",
  });

  bindEvents();
}

function bindEvents() {
  document.getElementById("borrowStatusFilterAdmin")?.addEventListener("change", (event) => {
    state.status = event.target.value;
    state.borrowPageNo = 1;
    loadPage();
  });

  document.querySelectorAll("[data-action='borrow']").forEach((button) => {
    button.addEventListener("click", () => {
      state.createBorrowReservationId = Number(button.dataset.id);
      const selected = state.approvedReservations.find((item) => item.id === state.createBorrowReservationId);
      const modalElement = document.getElementById("createBorrowModal");
      modalElement.querySelector("input[name='reservationId']").value = String(selected?.id || "");
      bootstrapModal("createBorrowModal")?.show();
    });
  });

  document.querySelectorAll("[data-action='detail']").forEach((button) => {
    button.addEventListener("click", () => {
      state.selectedBorrowId = Number(button.dataset.id);
      render();
    });
  });

  document.querySelectorAll("[data-action='return']").forEach((button) => {
    button.addEventListener("click", () => {
      state.selectedBorrowId = Number(button.dataset.id);
      const selected = getSelectedBorrow();
      const modalElement = document.getElementById("returnBorrowModal");
      modalElement.querySelector("input[name='borrowRecordId']").value = String(selected?.id || "");
      bootstrapModal("returnBorrowModal")?.show();
    });
  });

  const paginationRoot = document.querySelector(".pagination");
  if (paginationRoot) {
    bindPagination(paginationRoot, (nextPage) => {
      state.borrowPageNo = nextPage;
      loadPage();
    });
  }

  document.getElementById("createBorrowForm")?.addEventListener("submit", async (event) => {
    event.preventDefault();
    const formData = new FormData(event.currentTarget);
    const feedback = document.getElementById("createBorrowFeedback");
    setFeedback(feedback, "");
    try {
      await borrowApi.create({
        reservationId: Number(formData.get("reservationId")),
        borrowedAt: String(formData.get("borrowedAt") || "").trim(),
        dueAt: String(formData.get("dueAt") || "").trim(),
      });
      bootstrapModal("createBorrowModal")?.hide();
      state.pageFeedback = {
        message: "借出办理成功，预约状态已经同步推进到借用阶段。",
        type: "success",
      };
      loadPage();
    } catch (error) {
      setFeedback(feedback, safeErrorMessage(error, "办理借出失败。"));
    }
  });

  document.getElementById("returnBorrowForm")?.addEventListener("submit", async (event) => {
    event.preventDefault();
    const formData = new FormData(event.currentTarget);
    const feedback = document.getElementById("returnBorrowFeedback");
    setFeedback(feedback, "");
    try {
      await borrowApi.returnBorrow(Number(formData.get("borrowRecordId")), {
        returnedAt: String(formData.get("returnedAt") || "").trim(),
        returnNote: String(formData.get("returnNote") || "").trim(),
      });
      bootstrapModal("returnBorrowModal")?.hide();
      state.pageFeedback = {
        message: "归还办理成功，借用记录和预约记录都已同步闭环。",
        type: "success",
      };
      loadPage();
    } catch (error) {
      setFeedback(feedback, safeErrorMessage(error, "办理归还失败。"));
    }
  });
}

async function loadPage() {
  try {
    const [approvedReservationsPage, borrowPage] = await Promise.all([
      reservationApi.getAdminPage({
        pageNo: 1,
        pageSize: 8,
        status: "APPROVED",
      }),
      borrowApi.getAdminPage({
        pageNo: state.borrowPageNo,
        pageSize: state.pageSize,
        status: state.status || undefined,
      }),
    ]);

    state.approvedReservations = approvedReservationsPage.items;
    state.borrowPage = borrowPage;
    if (!state.selectedBorrowId || !state.borrowPage.items.some((item) => item.id === state.selectedBorrowId)) {
      state.selectedBorrowId = state.borrowPage.items[0]?.id || null;
    }
    render();
  } catch (error) {
    state.pageFeedback = { message: safeErrorMessage(error, "借还管理数据加载失败。"), type: "error" };
    state.approvedReservations = [];
    state.borrowPage = { pageNo: 1, pageSize: state.pageSize, total: 0, items: [] };
    render();
  }
}

loadPage();
