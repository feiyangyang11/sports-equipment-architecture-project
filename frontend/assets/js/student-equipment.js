import { DEFAULT_PAGE_SIZE } from "./config.js";
import { equipmentApi, reservationApi } from "./api.js";
import { requireRole } from "./guard.js";
import { mountShell } from "./shell.js";
import { bindPagination, escapeHtml, renderPagination, renderStatusPill, sumBy } from "./utils.js";
import { bootstrapModal, safeErrorMessage, setFeedback } from "./ui.js";

const currentUser = requireRole("STUDENT");

const state = {
  categories: [],
  pageNo: 1,
  pageSize: DEFAULT_PAGE_SIZE,
  categoryId: "",
  pageResult: { pageNo: 1, pageSize: DEFAULT_PAGE_SIZE, total: 0, items: [] },
  selectedEquipmentId: null,
  modalEquipmentId: null,
  pageFeedback: { message: "", type: "success" },
};

function getSelectedEquipment() {
  return (
    state.pageResult.items.find((item) => item.id === state.selectedEquipmentId) ||
    state.pageResult.items[0] ||
    null
  );
}

function renderEquipmentCards(items) {
  if (!items.length) {
    return `<div class="empty-state">当前筛选条件下没有器材记录，可以切换分类重新查看。</div>`;
  }

  return `
    <div class="stack">
      ${items.map((equipment) => `
        <article class="list-card">
          <div class="list-card__title">${escapeHtml(equipment.equipmentName)}</div>
          <p class="list-card__meta">${escapeHtml(equipment.equipmentCode)} · ${escapeHtml(equipment.specification || "未填写规格")}</p>
          <div class="chips" style="margin-top: 12px;">
            <span class="chip">库存 ${equipment.availableStock} / ${equipment.totalStock}</span>
            <span class="chip chip--muted">${escapeHtml(equipment.storageLocation || "位置未填写")}</span>
            ${renderStatusPill(equipment.status)}
          </div>
          <div class="list-card__footer">
            <div class="button-row">
              <button class="btn btn--ghost" data-action="detail" data-id="${equipment.id}">查看详情</button>
              <button class="btn btn--primary" data-action="reserve" data-id="${equipment.id}">发起预约</button>
            </div>
          </div>
        </article>
      `).join("")}
    </div>
  `;
}

function renderDetailCard(equipment) {
  if (!equipment) {
    return `
      <section class="panel">
        <div class="empty-state">当前页没有可展示的器材详情。</div>
      </section>
    `;
  }

  return `
    <section class="panel detail-card">
      <div class="panel__header">
        <div>
          <h3 class="panel__title">${escapeHtml(equipment.equipmentName)}</h3>
          <p class="panel__subtitle">${escapeHtml(equipment.equipmentCode)} · ${escapeHtml(equipment.specification || "未填写规格")}</p>
        </div>
        ${renderStatusPill(equipment.status)}
      </div>
      <div class="detail-list">
        <div class="detail-item">
          <p class="detail-item__label">存放位置</p>
          <p class="detail-item__value">${escapeHtml(equipment.storageLocation || "未填写")}</p>
        </div>
        <div class="detail-item">
          <p class="detail-item__label">库存情况</p>
          <p class="detail-item__value">可用 ${equipment.availableStock} / 总量 ${equipment.totalStock}</p>
        </div>
        <div class="detail-item">
          <p class="detail-item__label">使用提示</p>
          <p class="detail-item__value">请结合库存、状态和存放位置选择合适的器材后再提交预约。</p>
        </div>
      </div>
      <div class="button-row" style="margin-top: 18px;">
        <button class="btn btn--primary" data-action="reserve" data-id="${equipment.id}">预约这件器材</button>
      </div>
    </section>
  `;
}

function renderReserveModal(equipment) {
  return `
    <div class="modal fade" id="reservationModal" tabindex="-1" aria-hidden="true">
      <div class="modal-dialog modal-dialog-centered">
        <div class="modal-content" style="border-radius: 22px; border: 1px solid rgba(16, 32, 51, 0.1);">
          <div class="modal-header" style="border-bottom: 1px solid rgba(16, 32, 51, 0.08);">
            <div>
              <h5 class="modal-title">创建预约申请</h5>
              <div class="text-secondary small" id="reservationTargetName">${escapeHtml(equipment?.equipmentName || "未选择器材")}</div>
            </div>
            <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="关闭"></button>
          </div>
          <form id="reservationForm">
            <div class="modal-body">
              <div class="feedback" id="reservationFeedback"></div>
              <input type="hidden" name="equipmentId" value="${equipment?.id || ""}" />
              <div class="field">
                <label for="reservationStartAt">开始时间</label>
                <input id="reservationStartAt" name="reservationStartAt" type="text" placeholder="2026-06-06 10:00:00" />
              </div>
              <div class="field" style="margin-top: 12px;">
                <label for="reservationEndAt">结束时间</label>
                <input id="reservationEndAt" name="reservationEndAt" type="text" placeholder="2026-06-06 11:00:00" />
              </div>
              <div class="field" style="margin-top: 12px;">
                <label for="quantity">数量</label>
                <input id="quantity" name="quantity" type="number" min="1" value="1" />
              </div>
              <div class="field" style="margin-top: 12px;">
                <label for="requestNote">预约说明</label>
                <textarea id="requestNote" name="requestNote" placeholder="例如：班级训练、社团活动、个人练习。"></textarea>
              </div>
            </div>
            <div class="modal-footer" style="border-top: 1px solid rgba(16, 32, 51, 0.08);">
              <button type="button" class="btn btn--ghost" data-bs-dismiss="modal">取消</button>
              <button type="submit" class="btn btn--primary">提交预约</button>
            </div>
          </form>
        </div>
      </div>
    </div>
  `;
}

function render() {
  const equipment = getSelectedEquipment();
  const totalItems = state.pageResult.items.length;
  const totalAvailable = sumBy(state.pageResult.items, (item) => item.availableStock);
  const totalStock = sumBy(state.pageResult.items, (item) => item.totalStock);

  const stats = `
    <section class="summary-grid fade-up">
      <article class="stat-card">
        <p class="stat-card__label">当前页器材数</p>
        <p class="stat-card__value">${totalItems}</p>
      </article>
      <article class="stat-card">
        <p class="stat-card__label">当前页可用库存</p>
        <p class="stat-card__value">${totalAvailable}</p>
      </article>
      <article class="stat-card">
        <p class="stat-card__label">当前页总库存</p>
        <p class="stat-card__value">${totalStock}</p>
      </article>
      <article class="stat-card">
        <p class="stat-card__label">分类筛选</p>
        <p class="stat-card__value">${state.categoryId ? "已开启" : "全部"}</p>
      </article>
    </section>
  `;

  const main = `
    <section class="panel">
      <div class="panel__header">
        <div>
          <h3 class="panel__title">器材列表</h3>
          <p class="panel__subtitle">可按分类筛选器材，查看库存与详情，并直接发起预约申请。</p>
        </div>
      </div>
      <div class="feedback ${state.pageFeedback.message ? `feedback--${state.pageFeedback.type} is-visible` : ""}" id="equipmentPageFeedback">${escapeHtml(state.pageFeedback.message)}</div>
      <div class="toolbar">
        <div class="field">
          <label for="categoryFilter">分类筛选</label>
          <select id="categoryFilter">
            <option value="">全部分类</option>
            ${state.categories.map((category) => `
              <option value="${category.id}" ${String(category.id) === String(state.categoryId) ? "selected" : ""}>${escapeHtml(category.categoryName)}</option>
            `).join("")}
          </select>
        </div>
      </div>
      ${renderEquipmentCards(state.pageResult.items)}
      ${renderPagination(state.pageResult)}
    </section>
    ${renderReserveModal(equipment)}
  `;

  mountShell({
    user: currentUser,
    role: "STUDENT",
    navKey: "student-equipment",
    eyebrow: "Student Equipment Hub",
    title: "器材总览",
    lead: "在这里浏览可用器材、了解库存情况，并提交预约申请。",
    stats,
    main,
    aside: renderDetailCard(equipment),
    topBadge: "学生端",
  });

  bindEvents();
}

function bindEvents() {
  document.getElementById("categoryFilter")?.addEventListener("change", (event) => {
    state.categoryId = event.target.value;
    state.pageNo = 1;
    loadPage();
  });

  document.querySelectorAll("[data-action='detail']").forEach((button) => {
    button.addEventListener("click", () => {
      state.selectedEquipmentId = Number(button.dataset.id);
      render();
    });
  });

  document.querySelectorAll("[data-action='reserve']").forEach((button) => {
    button.addEventListener("click", () => {
      state.modalEquipmentId = Number(button.dataset.id);
      const target = state.pageResult.items.find((item) => item.id === state.modalEquipmentId) || getSelectedEquipment();
      const modalElement = document.getElementById("reservationModal");
      modalElement.querySelector("input[name='equipmentId']").value = String(target?.id || "");
      modalElement.querySelector("#reservationTargetName").textContent = target?.equipmentName || "未选择器材";
      bootstrapModal("reservationModal")?.show();
    });
  });

  const paginationRoot = document.querySelector(".pagination");
  if (paginationRoot) {
    bindPagination(paginationRoot, (nextPage) => {
      state.pageNo = nextPage;
      loadPage();
    });
  }

  document.getElementById("reservationForm")?.addEventListener("submit", async (event) => {
    event.preventDefault();
    const form = event.currentTarget;
    const feedback = document.getElementById("reservationFeedback");
    setFeedback(feedback, "");

    const formData = new FormData(form);
    const payload = {
      equipmentId: Number(formData.get("equipmentId")),
      reservationStartAt: String(formData.get("reservationStartAt") || "").trim(),
      reservationEndAt: String(formData.get("reservationEndAt") || "").trim(),
      quantity: Number(formData.get("quantity") || 0),
      requestNote: String(formData.get("requestNote") || "").trim(),
    };

    try {
      await reservationApi.create(payload);
      bootstrapModal("reservationModal")?.hide();
      state.pageFeedback = {
        message: "预约申请已提交，请在“我的预约”中查看审核进度。",
        type: "success",
      };
      form.reset();
      loadPage();
    } catch (error) {
      setFeedback(feedback, safeErrorMessage(error, "预约创建失败，请稍后重试。"));
    }
  });
}

async function loadPage() {
  try {
    const [categoriesResponse, pageResponse] = await Promise.all([
      equipmentApi.getCategories(),
      equipmentApi.getPage({
        pageNo: state.pageNo,
        pageSize: state.pageSize,
        categoryId: state.categoryId || undefined,
      }),
    ]);
    state.categories = categoriesResponse.items;
    state.pageResult = pageResponse;
    if (!state.selectedEquipmentId || !state.pageResult.items.some((item) => item.id === state.selectedEquipmentId)) {
      state.selectedEquipmentId = state.pageResult.items[0]?.id || null;
    }
    render();
  } catch (error) {
    state.pageResult = { pageNo: 1, pageSize: state.pageSize, total: 0, items: [] };
    state.pageFeedback = {
      message: safeErrorMessage(error, "器材数据加载失败。"),
      type: "error",
    };
    render();
  }
}

loadPage();
