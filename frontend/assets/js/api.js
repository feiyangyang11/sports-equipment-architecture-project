import { API_BASE_URL, USE_MOCK } from "./config.js";
import { getToken } from "./auth.js";
import * as mockApi from "./mock-api.js";

async function parseResponse(response) {
  const contentType = response.headers.get("content-type") || "";
  if (contentType.includes("application/json")) {
    return response.json();
  }
  return response.text();
}

async function request(path, { method = "GET", body, auth = true } = {}) {
  const headers = {};
  if (body !== undefined) {
    headers["Content-Type"] = "application/json";
  }
  if (auth) {
    const token = getToken();
    if (token) {
      headers.Authorization = `Bearer ${token}`;
    }
  }

  const response = await fetch(`${API_BASE_URL}${path}`, {
    method,
    headers,
    body: body !== undefined ? JSON.stringify(body) : undefined,
  });

  const payload = await parseResponse(response);
  if (!response.ok) {
    throw {
      status: response.status,
      message: typeof payload === "string" ? payload : payload?.message || "request failed",
    };
  }

  return payload;
}

export const authApi = {
  login(username, password) {
    if (USE_MOCK) {
      return mockApi.login({ username, password });
    }
    return request("/api/login", {
      method: "POST",
      auth: false,
      body: { username, password },
    });
  },
  me() {
    if (USE_MOCK) {
      return mockApi.getMe({ token: getToken() });
    }
    return request("/api/me");
  },
  logout() {
    if (USE_MOCK) {
      return mockApi.logout({ token: getToken() });
    }
    return request("/api/logout", { method: "POST" });
  },
};

export const equipmentApi = {
  getCategories() {
    if (USE_MOCK) {
      return mockApi.getEquipmentCategories();
    }
    return request("/api/equipment-categories", { auth: false });
  },
  getPage({ pageNo, pageSize, categoryId }) {
    if (USE_MOCK) {
      return mockApi.getEquipmentPage({ pageNo, pageSize, categoryId });
    }
    const query = new URLSearchParams({
      pageNo: String(pageNo),
      pageSize: String(pageSize),
    });
    if (categoryId) {
      query.set("categoryId", String(categoryId));
    }
    return request(`/api/equipment?${query.toString()}`, { auth: false });
  },
  getById(id) {
    if (USE_MOCK) {
      return mockApi.getEquipmentByIdApi({ id });
    }
    return request(`/api/equipment/${id}`, { auth: false });
  },
};

export const reservationApi = {
  create(payload) {
    if (USE_MOCK) {
      return mockApi.createReservation({ token: getToken(), payload });
    }
    return request("/api/reservations", { method: "POST", body: payload });
  },
  getMine({ pageNo, pageSize, status }) {
    if (USE_MOCK) {
      return mockApi.getMyReservations({
        token: getToken(),
        pageNo,
        pageSize,
        status,
      });
    }
    const query = new URLSearchParams({
      pageNo: String(pageNo),
      pageSize: String(pageSize),
    });
    if (status) {
      query.set("status", status);
    }
    return request(`/api/reservations/my?${query.toString()}`);
  },
  getMineDetail(id) {
    if (USE_MOCK) {
      return mockApi.getMyReservationById({ token: getToken(), id });
    }
    return request(`/api/reservations/my/${id}`);
  },
  cancelMine(id, payload) {
    if (USE_MOCK) {
      return mockApi.cancelMyReservation({ token: getToken(), id, payload });
    }
    return request(`/api/reservations/my/${id}/cancel`, {
      method: "POST",
      body: payload,
    });
  },
  getAdminPage({ pageNo, pageSize, status }) {
    if (USE_MOCK) {
      return mockApi.getAdminReservations({
        token: getToken(),
        pageNo,
        pageSize,
        status,
      });
    }
    const query = new URLSearchParams({
      pageNo: String(pageNo),
      pageSize: String(pageSize),
    });
    if (status) {
      query.set("status", status);
    }
    return request(`/api/admin/reservations?${query.toString()}`);
  },
  approve(id, payload) {
    if (USE_MOCK) {
      return mockApi.reviewReservation({
        token: getToken(),
        id,
        action: "approve",
        payload,
      });
    }
    return request(`/api/admin/reservations/${id}/approve`, {
      method: "POST",
      body: payload,
    });
  },
  reject(id, payload) {
    if (USE_MOCK) {
      return mockApi.reviewReservation({
        token: getToken(),
        id,
        action: "reject",
        payload,
      });
    }
    return request(`/api/admin/reservations/${id}/reject`, {
      method: "POST",
      body: payload,
    });
  },
};

export const borrowApi = {
  create(payload) {
    if (USE_MOCK) {
      return mockApi.createBorrow({ token: getToken(), payload });
    }
    return request("/api/admin/borrows", { method: "POST", body: payload });
  },
  getMine({ pageNo, pageSize, status }) {
    if (USE_MOCK) {
      return mockApi.getMyBorrows({ token: getToken(), pageNo, pageSize, status });
    }
    const query = new URLSearchParams({
      pageNo: String(pageNo),
      pageSize: String(pageSize),
    });
    if (status) {
      query.set("status", status);
    }
    return request(`/api/borrows/my?${query.toString()}`);
  },
  getMineDetail(id) {
    if (USE_MOCK) {
      return mockApi.getMyBorrowById({ token: getToken(), id });
    }
    return request(`/api/borrows/my/${id}`);
  },
  getAdminPage({ pageNo, pageSize, status }) {
    if (USE_MOCK) {
      return mockApi.getAdminBorrows({
        token: getToken(),
        pageNo,
        pageSize,
        status,
      });
    }
    const query = new URLSearchParams({
      pageNo: String(pageNo),
      pageSize: String(pageSize),
    });
    if (status) {
      query.set("status", status);
    }
    return request(`/api/admin/borrows?${query.toString()}`);
  },
  getAdminDetail(id) {
    if (USE_MOCK) {
      return mockApi.getAdminBorrowById({ token: getToken(), id });
    }
    return request(`/api/admin/borrows/${id}`);
  },
  returnBorrow(id, payload) {
    if (USE_MOCK) {
      return mockApi.returnBorrow({ token: getToken(), id, payload });
    }
    return request(`/api/admin/borrows/${id}/return`, {
      method: "POST",
      body: payload,
    });
  },
};
