export function escapeHtml(value) {
  return String(value ?? "")
      .replaceAll("&", "&amp;")
      .replaceAll("<", "&lt;")
      .replaceAll(">", "&gt;")
      .replaceAll('"', "&quot;")
      .replaceAll("'", "&#39;");
}

export function formatDateTime(value) {
  if (!value) {
    return "—";
  }
  return value.replace("T", " ");
}

export function getStatusTone(status) {
  const normalized = String(status || "").toUpperCase();
  if (["AVAILABLE", "ACTIVE", "APPROVED", "BORROWING"].includes(normalized)) {
    return "available";
  }
  if (["PENDING", "OVERDUE"].includes(normalized)) {
    return "pending";
  }
  if (["REJECTED", "CANCELED", "DISABLED"].includes(normalized)) {
    return "rejected";
  }
  if (["RETURNED", "COMPLETED", "CLOSED"].includes(normalized)) {
    return "returned";
  }
  if (["MAINTENANCE"].includes(normalized)) {
    return "maintenance";
  }
  return "muted";
}

export function renderStatusPill(status) {
  const tone = getStatusTone(status);
  return `<span class="status-pill status-pill--${tone}">${escapeHtml(status || "UNKNOWN")}</span>`;
}

export function renderPagination({ pageNo, pageSize, total }) {
  const totalPages = Math.max(1, Math.ceil(total / pageSize));
  return `
    <div class="pagination">
      <div class="pagination__summary">第 ${pageNo} / ${totalPages} 页，共 ${total} 条记录</div>
      <div class="pagination__buttons">
        <button class="btn btn--ghost" data-page="${Math.max(1, pageNo - 1)}" ${pageNo <= 1 ? "disabled" : ""}>上一页</button>
        <button class="btn btn--ghost" data-page="${Math.min(totalPages, pageNo + 1)}" ${pageNo >= totalPages ? "disabled" : ""}>下一页</button>
      </div>
    </div>
  `;
}

export function bindPagination(container, onChange) {
  container.querySelectorAll("[data-page]").forEach((button) => {
    button.addEventListener("click", () => {
      const nextPage = Number(button.dataset.page);
      onChange(nextPage);
    });
  });
}

export function sumBy(items, projector) {
  return items.reduce((total, item) => total + projector(item), 0);
}
