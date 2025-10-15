/*
 * Inventory Management System
 * Desktop Application using C and SQLite3
 * Windows API GUI Implementation
 * Enhanced with Dialog Boxes and Sample Data
 */

#include <windows.h>
#include <commctrl.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "comctl32.lib")

// Control IDs
#define ID_LISTVIEW_PRODUCTS 1001
#define ID_LISTVIEW_SALES 1002
#define ID_BTN_ADD 1003
#define ID_BTN_UPDATE 1004
#define ID_BTN_DELETE 1005
#define ID_BTN_REFRESH 1006
#define ID_BTN_PURCHASE 1007
#define ID_BTN_VIEW_SALES 1008
#define ID_BTN_SEARCH 1009
#define ID_EDIT_SEARCH 1010
#define ID_TAB_CONTROL 1011

// Dialog control IDs
#define IDC_EDIT_NAME 2001
#define IDC_EDIT_QUANTITY 2002
#define IDC_EDIT_PRICE 2003
#define IDC_EDIT_PURCHASE_QTY 2004
#define IDC_BTN_OK 2005
#define IDC_BTN_CANCEL 2006

// Global variables
HWND hListViewProducts, hListViewSales, hTabControl;
HWND hEditSearch;
sqlite3 *db;
HINSTANCE hInst;
HWND g_hCurrentDialog = NULL;
int g_dialogResult = 0;
HWND g_hEditName = NULL;
HWND g_hEditQty = NULL;
HWND g_hEditPrice = NULL;
HWND g_hEditPurchaseQty = NULL;

// Global variables for dialog communication
char g_dialogName[256] = {0};
char g_dialogQty[50] = {0};
char g_dialogPrice[50] = {0};
char g_dialogId[20] = {0};

// Dialog Window Procedure
LRESULT CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                EndDialog(hwnd, LOWORD(wParam));
                return TRUE;
            }
            break;

        case WM_CLOSE:
            EndDialog(hwnd, IDCANCEL);
            return TRUE;
    }
    return FALSE;
}

// Dialog data structure
typedef struct {
    char name[256];
    int quantity;
    double price;
    int productId;
    int isUpdate;
} ProductDialogData;

typedef struct {
    int quantity;
    double price;
    char productName[256];
} PurchaseDialogData;

ProductDialogData g_productData = {0};
PurchaseDialogData g_purchaseData = {0};

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK ProductDialogProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK PurchaseDialogProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SimpleDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK) {
                g_dialogResult = IDOK;

                return 0;
            }
            else if (LOWORD(wParam) == IDCANCEL) {
                g_dialogResult = IDCANCEL;
                g_hCurrentDialog = NULL;
                DestroyWindow(hwnd);
                return 0;
            }
            break;

        case WM_CLOSE:
            g_dialogResult = IDCANCEL;
            g_hCurrentDialog = NULL;
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            // Don't call PostQuitMessage here - it closes the entire app!
            // Just return and let the message loop exit naturally
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void InitDatabase();
void InsertSampleData();
void CreateControls(HWND hwnd);
void LoadProducts();
void LoadSales();
void AddProduct(HWND hwnd);
void UpdateProduct(HWND hwnd);
void DeleteProduct(HWND hwnd);
void PurchaseProduct(HWND hwnd);
void SearchProducts(HWND hwnd);
void ShowError(const char* message);
void ShowSuccess(const char* message);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
    hInst = hInstance;

    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES;
    InitCommonControlsEx(&icex);

    // Initialize database
    InitDatabase();

    // Register window class
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "InventoryApp";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Create main window
    HWND hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        "InventoryApp",
        "Inventory Management System",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1000, 700,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) {
        MessageBox(NULL, "Window Creation Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    sqlite3_close(db);
    return msg.wParam;
}

void InitDatabase() {
    int rc = sqlite3_open("inventory.db", &db);
    if (rc) {
        MessageBox(NULL, "Cannot open database", "Error", MB_OK | MB_ICONERROR);
        exit(1);
    }

    // Create products table
    const char* sqlProducts =
        "CREATE TABLE IF NOT EXISTS products ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL UNIQUE,"
        "quantity INTEGER NOT NULL DEFAULT 0,"
        "price REAL NOT NULL,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP);";

    // Create sales table
    const char* sqlSales =
        "CREATE TABLE IF NOT EXISTS sales ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "product_id INTEGER NOT NULL,"
        "product_name TEXT NOT NULL,"
        "quantity_sold INTEGER NOT NULL,"
        "total_amount REAL NOT NULL,"
        "sale_date DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (product_id) REFERENCES products(id));";

    char* errMsg = 0;
    rc = sqlite3_exec(db, sqlProducts, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        MessageBox(NULL, errMsg, "Database Error", MB_OK | MB_ICONERROR);
        sqlite3_free(errMsg);
        return;
    }

    rc = sqlite3_exec(db, sqlSales, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        MessageBox(NULL, errMsg, "Database Error", MB_OK | MB_ICONERROR);
        sqlite3_free(errMsg);
        return;
    }

    // Insert sample data
    InsertSampleData();
}

void InsertSampleData() {
    // Check if data already exists
    const char* checkSql = "SELECT COUNT(*) FROM products";
    sqlite3_stmt* stmt;
    int count = 0;

    if (sqlite3_prepare_v2(db, checkSql, -1, &stmt, 0) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
    }
    sqlite3_finalize(stmt);

    if (count > 0) {
        return; // Data already exists
    }

    // Sample products data
    const char* sampleProducts[][3] = {
        {"Laptop Dell XPS 13", "15", "8500.00"},
        {"iPhone 14 Pro", "25", "6200.00"},
        {"Samsung Galaxy S23", "30", "4800.00"},
        {"HP LaserJet Printer", "12", "1500.00"},
        {"Logitech Wireless Mouse", "50", "250.00"},
        {"Mechanical Keyboard", "40", "450.00"},
        {"27-inch Monitor", "20", "2200.00"},
        {"USB-C Hub", "60", "180.00"},
        {"External Hard Drive 2TB", "35", "650.00"},
        {"Wireless Headphones", "45", "380.00"},
        {"Webcam HD 1080p", "28", "320.00"},
        {"Gaming Mouse Pad", "100", "85.00"},
        {"Phone Case", "150", "65.00"},
        {"Screen Protector", "200", "45.00"},
        {"Power Bank 20000mAh", "55", "280.00"}
    };

    const char* sql = "INSERT INTO products (name, quantity, price) VALUES (?, ?, ?)";

    for (int i = 0; i < 15; i++) {
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, sampleProducts[i][0], -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 2, atoi(sampleProducts[i][1]));
            sqlite3_bind_double(stmt, 3, atof(sampleProducts[i][2]));
            sqlite3_step(stmt);
        }
        sqlite3_finalize(stmt);
    }
}

void CreateControls(HWND hwnd) {
    // Create Tab Control
    hTabControl = CreateWindowEx(
        0, WC_TABCONTROL, "",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
        10, 10, 960, 550,
        hwnd, (HMENU)ID_TAB_CONTROL, hInst, NULL
    );

    // Add tabs
    TCITEM tie;
    tie.mask = TCIF_TEXT;
    tie.pszText = "Products";
    TabCtrl_InsertItem(hTabControl, 0, &tie);
    tie.pszText = "Sales Report";
    TabCtrl_InsertItem(hTabControl, 1, &tie);

    // Search box
    CreateWindow("STATIC", "Search:", WS_CHILD | WS_VISIBLE,
                 20, 50, 60, 20, hwnd, NULL, hInst, NULL);

    hEditSearch = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "",
                                  WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                  85, 48, 200, 22, hwnd, (HMENU)ID_EDIT_SEARCH, hInst, NULL);

    CreateWindow("BUTTON", "Search", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                 295, 47, 80, 24, hwnd, (HMENU)ID_BTN_SEARCH, hInst, NULL);

    // Products ListView
    hListViewProducts = CreateWindowEx(
        WS_EX_CLIENTEDGE, WC_LISTVIEW, "",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
        20, 80, 940, 450,
        hwnd, (HMENU)ID_LISTVIEW_PRODUCTS, hInst, NULL
    );

    ListView_SetExtendedListViewStyle(hListViewProducts,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    // Add columns to Products ListView
    LVCOLUMN lvc;
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    lvc.pszText = "ID";
    lvc.cx = 60;
    ListView_InsertColumn(hListViewProducts, 0, &lvc);

    lvc.pszText = "Product Name";
    lvc.cx = 350;
    ListView_InsertColumn(hListViewProducts, 1, &lvc);

    lvc.pszText = "Quantity";
    lvc.cx = 150;
    ListView_InsertColumn(hListViewProducts, 2, &lvc);

    lvc.pszText = "Price (K)";
    lvc.cx = 150;
    ListView_InsertColumn(hListViewProducts, 3, &lvc);

    lvc.pszText = "Created Date";
    lvc.cx = 200;
    ListView_InsertColumn(hListViewProducts, 4, &lvc);

    // Sales ListView
    hListViewSales = CreateWindowEx(
        WS_EX_CLIENTEDGE, WC_LISTVIEW, "",
        WS_CHILD | LVS_REPORT | LVS_SINGLESEL,
        20, 80, 940, 450,
        hwnd, (HMENU)ID_LISTVIEW_SALES, hInst, NULL
    );

    ListView_SetExtendedListViewStyle(hListViewSales,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    lvc.pszText = "Sale ID";
    lvc.cx = 80;
    ListView_InsertColumn(hListViewSales, 0, &lvc);

    lvc.pszText = "Product Name";
    lvc.cx = 300;
    ListView_InsertColumn(hListViewSales, 1, &lvc);

    lvc.pszText = "Quantity Sold";
    lvc.cx = 150;
    ListView_InsertColumn(hListViewSales, 2, &lvc);

    lvc.pszText = "Total Amount (K)";
    lvc.cx = 150;
    ListView_InsertColumn(hListViewSales, 3, &lvc);

    lvc.pszText = "Sale Date";
    lvc.cx = 230;
    ListView_InsertColumn(hListViewSales, 4, &lvc);

    // Buttons
    int btnY = 570;
    CreateWindow("BUTTON", "Add Product", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                 20, btnY, 120, 30, hwnd, (HMENU)ID_BTN_ADD, hInst, NULL);

    CreateWindow("BUTTON", "Update Product", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                 150, btnY, 120, 30, hwnd, (HMENU)ID_BTN_UPDATE, hInst, NULL);

    CreateWindow("BUTTON", "Delete Product", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                 280, btnY, 120, 30, hwnd, (HMENU)ID_BTN_DELETE, hInst, NULL);

    CreateWindow("BUTTON", "Purchase", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                 410, btnY, 120, 30, hwnd, (HMENU)ID_BTN_PURCHASE, hInst, NULL);

    CreateWindow("BUTTON", "Refresh", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                 540, btnY, 120, 30, hwnd, (HMENU)ID_BTN_REFRESH, hInst, NULL);

    CreateWindow("BUTTON", "View Sales", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                 670, btnY, 120, 30, hwnd, (HMENU)ID_BTN_VIEW_SALES, hInst, NULL);

    LoadProducts();
}

INT_PTR CALLBACK ProductDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG: {
            // Set dialog title
            SetWindowText(hDlg, g_productData.isUpdate ? "Update Product" : "Add Product");

            // Set initial values
            SetDlgItemText(hDlg, IDC_EDIT_NAME, g_productData.name);
            SetDlgItemInt(hDlg, IDC_EDIT_QUANTITY, g_productData.quantity, FALSE);

            char priceStr[50];
            sprintf(priceStr, "%.2f", g_productData.price);
            SetDlgItemText(hDlg, IDC_EDIT_PRICE, priceStr);

            return TRUE;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK) {
                // Get values
                GetDlgItemText(hDlg, IDC_EDIT_NAME, g_productData.name, 256);
                g_productData.quantity = GetDlgItemInt(hDlg, IDC_EDIT_QUANTITY, NULL, FALSE);

                char priceStr[50];
                GetDlgItemText(hDlg, IDC_EDIT_PRICE, priceStr, 50);
                g_productData.price = atof(priceStr);

                // Validate
                if (strlen(g_productData.name) == 0) {
                    MessageBox(hDlg, "Please enter product name", "Validation Error", MB_OK | MB_ICONWARNING);
                    return TRUE;
                }

                if (g_productData.price <= 0) {
                    MessageBox(hDlg, "Price must be greater than 0", "Validation Error", MB_OK | MB_ICONWARNING);
                    return TRUE;
                }

                EndDialog(hDlg, IDOK);
                return TRUE;
            }
            else if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, IDCANCEL);
                return TRUE;
            }
            break;
    }
    return FALSE;
}

INT_PTR CALLBACK PurchaseDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG: {
            char info[512];
            sprintf(info, "Product: %s\nPrice: K%.2f per unit",
                    g_purchaseData.productName, g_purchaseData.price);
            SetDlgItemText(hDlg, IDC_EDIT_NAME, info);
            SetDlgItemInt(hDlg, IDC_EDIT_PURCHASE_QTY, 1, FALSE);
            return TRUE;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK) {
                g_purchaseData.quantity = GetDlgItemInt(hDlg, IDC_EDIT_PURCHASE_QTY, NULL, FALSE);

                if (g_purchaseData.quantity <= 0) {
                    MessageBox(hDlg, "Quantity must be greater than 0", "Validation Error", MB_OK | MB_ICONWARNING);
                    return TRUE;
                }

                EndDialog(hDlg, IDOK);
                return TRUE;
            }
            else if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, IDCANCEL);
                return TRUE;
            }
            break;
    }
    return FALSE;
}

void LoadProducts() {
    ListView_DeleteAllItems(hListViewProducts);

    const char* sql = "SELECT id, name, quantity, price, created_at FROM products ORDER BY id";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
        int row = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            LVITEM lvi = {0};
            char buffer[256];

            lvi.mask = LVIF_TEXT;
            lvi.iItem = row;

            sprintf(buffer, "%d", sqlite3_column_int(stmt, 0));
            lvi.pszText = buffer;
            ListView_InsertItem(hListViewProducts, &lvi);

            ListView_SetItemText(hListViewProducts, row, 1,
                (char*)sqlite3_column_text(stmt, 1));

            sprintf(buffer, "%d", sqlite3_column_int(stmt, 2));
            ListView_SetItemText(hListViewProducts, row, 2, buffer);

            sprintf(buffer, "%.2f", sqlite3_column_double(stmt, 3));
            ListView_SetItemText(hListViewProducts, row, 3, buffer);

            ListView_SetItemText(hListViewProducts, row, 4,
                (char*)sqlite3_column_text(stmt, 4));

            row++;
        }
    }
    sqlite3_finalize(stmt);
}

void LoadSales() {
    ListView_DeleteAllItems(hListViewSales);

    const char* sql = "SELECT id, product_name, quantity_sold, total_amount, sale_date FROM sales ORDER BY id DESC";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
        int row = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            LVITEM lvi = {0};
            char buffer[256];

            lvi.mask = LVIF_TEXT;
            lvi.iItem = row;

            sprintf(buffer, "%d", sqlite3_column_int(stmt, 0));
            lvi.pszText = buffer;
            ListView_InsertItem(hListViewSales, &lvi);

            ListView_SetItemText(hListViewSales, row, 1,
                (char*)sqlite3_column_text(stmt, 1));

            sprintf(buffer, "%d", sqlite3_column_int(stmt, 2));
            ListView_SetItemText(hListViewSales, row, 2, buffer);

            sprintf(buffer, "%.2f", sqlite3_column_double(stmt, 3));
            ListView_SetItemText(hListViewSales, row, 3, buffer);

            ListView_SetItemText(hListViewSales, row, 4,
                (char*)sqlite3_column_text(stmt, 4));

            row++;
        }
    }
    sqlite3_finalize(stmt);
}

void AddProduct(HWND hwnd) {
    // Register dialog class (only once)
    static BOOL registered = FALSE;
    if (!registered) {
        WNDCLASSEX wc = {0};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.lpfnWndProc = SimpleDialogProc;
        wc.hInstance = hInst;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = "SimpleDialog";
        RegisterClassEx(&wc);
        registered = TRUE;
    }

    g_dialogResult = 0;

    // Create dialog window
    g_hCurrentDialog = CreateWindowEx(
        WS_EX_DLGMODALFRAME,
        "SimpleDialog",
        "Add Product",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        (GetSystemMetrics(SM_CXSCREEN) - 400) / 2,
        (GetSystemMetrics(SM_CYSCREEN) - 220) / 2,
        400, 220,
        hwnd, NULL, hInst, NULL
    );

    // Create controls
    CreateWindow("STATIC", "Product Name:", WS_CHILD | WS_VISIBLE,
                 20, 20, 120, 20, g_hCurrentDialog, NULL, hInst, NULL);
    g_hEditName = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "",
                                     WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_TABSTOP,
                                     150, 18, 220, 22, g_hCurrentDialog, (HMENU)IDC_EDIT_NAME, hInst, NULL);

    CreateWindow("STATIC", "Quantity:", WS_CHILD | WS_VISIBLE,
                 20, 55, 120, 20, g_hCurrentDialog, NULL, hInst, NULL);
    g_hEditQty = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "0",
                                    WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER | WS_TABSTOP,
                                    150, 53, 220, 22, g_hCurrentDialog, (HMENU)IDC_EDIT_QUANTITY, hInst, NULL);

    CreateWindow("STATIC", "Price (K):", WS_CHILD | WS_VISIBLE,
                 20, 90, 120, 20, g_hCurrentDialog, NULL, hInst, NULL);
    g_hEditPrice = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "0",
                                      WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_TABSTOP,
                                      150, 88, 220, 22, g_hCurrentDialog, (HMENU)IDC_EDIT_PRICE, hInst, NULL);

    CreateWindow("BUTTON", "OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP,
                 150, 135, 100, 30, g_hCurrentDialog, (HMENU)IDOK, hInst, NULL);
    CreateWindow("BUTTON", "Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                 270, 135, 100, 30, g_hCurrentDialog, (HMENU)IDCANCEL, hInst, NULL);

    ShowWindow(g_hCurrentDialog, SW_SHOW);
    SetFocus(g_hEditName);
    EnableWindow(hwnd, FALSE);

    // Message loop for modal dialog
    MSG msg;
    while (g_hCurrentDialog != NULL && GetMessage(&msg, NULL, 0, 0)) {
        if (!IsDialogMessage(g_hCurrentDialog, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Check if OK was clicked
        if (g_dialogResult == IDOK) {
            break;
        }
    }

    // Process the result BEFORE destroying dialog
    if (g_dialogResult == IDOK) {
        char name[256], qtyStr[50], priceStr[50];
        GetWindowText(g_hEditName, name, 256);
        GetWindowText(g_hEditQty, qtyStr, 50);
        GetWindowText(g_hEditPrice, priceStr, 50);

        // Now destroy the dialog
        if (g_hCurrentDialog) {
            DestroyWindow(g_hCurrentDialog);
            g_hCurrentDialog = NULL;
        }

        EnableWindow(hwnd, TRUE);
        SetForegroundWindow(hwnd);

        if (strlen(name) == 0) {
            MessageBox(hwnd, "Please enter product name", "Validation Error", MB_OK | MB_ICONWARNING);
            return;
        }

        int qty = atoi(qtyStr);
        double price = atof(priceStr);

        if (price <= 0) {
            MessageBox(hwnd, "Price must be greater than 0", "Validation Error", MB_OK | MB_ICONWARNING);
            return;
        }

        // Insert into database
        const char* sql = "INSERT INTO products (name, quantity, price) VALUES (?, ?, ?)";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, name, -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 2, qty);
            sqlite3_bind_double(stmt, 3, price);

            if (sqlite3_step(stmt) == SQLITE_DONE) {
                ShowSuccess("Product added successfully!");
                LoadProducts();
            } else {
                ShowError("Failed to add product. Name might already exist.");
            }
        }
        sqlite3_finalize(stmt);
    } else {
        // Cancel was clicked
        if (g_hCurrentDialog) {
            DestroyWindow(g_hCurrentDialog);
            g_hCurrentDialog = NULL;
        }
        EnableWindow(hwnd, TRUE);
        SetForegroundWindow(hwnd);
    }
}

void UpdateProduct(HWND hwnd) {
    int selectedIndex = ListView_GetNextItem(hListViewProducts, -1, LVNI_SELECTED);
    if (selectedIndex == -1) {
        ShowError("Please select a product to update.");
        return;
    }

    char id[20], name[256], qty[20], price[20];
    ListView_GetItemText(hListViewProducts, selectedIndex, 0, id, sizeof(id));
    ListView_GetItemText(hListViewProducts, selectedIndex, 1, name, sizeof(name));
    ListView_GetItemText(hListViewProducts, selectedIndex, 2, qty, sizeof(qty));
    ListView_GetItemText(hListViewProducts, selectedIndex, 3, price, sizeof(price));

    // Register dialog class
    static BOOL registered = FALSE;
    if (!registered) {
        WNDCLASSEX wc = {0};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.lpfnWndProc = SimpleDialogProc;
        wc.hInstance = hInst;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = "SimpleDialog";
        RegisterClassEx(&wc);
        registered = TRUE;
    }

    g_dialogResult = 0;

    // Create dialog
    g_hCurrentDialog = CreateWindowEx(
        WS_EX_DLGMODALFRAME,
        "SimpleDialog",
        "Update Product",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        (GetSystemMetrics(SM_CXSCREEN) - 400) / 2,
        (GetSystemMetrics(SM_CYSCREEN) - 220) / 2,
        400, 220,
        hwnd, NULL, hInst, NULL
    );

    CreateWindow("STATIC", "Product Name:", WS_CHILD | WS_VISIBLE,
                 20, 20, 120, 20, g_hCurrentDialog, NULL, hInst, NULL);
    g_hEditName = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", name,
                                     WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_TABSTOP,
                                     150, 18, 220, 22, g_hCurrentDialog, (HMENU)IDC_EDIT_NAME, hInst, NULL);

    CreateWindow("STATIC", "Quantity:", WS_CHILD | WS_VISIBLE,
                 20, 55, 120, 20, g_hCurrentDialog, NULL, hInst, NULL);
    g_hEditQty = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", qty,
                                    WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER | WS_TABSTOP,
                                    150, 53, 220, 22, g_hCurrentDialog, (HMENU)IDC_EDIT_QUANTITY, hInst, NULL);

    CreateWindow("STATIC", "Price (K):", WS_CHILD | WS_VISIBLE,
                 20, 90, 120, 20, g_hCurrentDialog, NULL, hInst, NULL);
    g_hEditPrice = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", price,
                                      WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_TABSTOP,
                                      150, 88, 220, 22, g_hCurrentDialog, (HMENU)IDC_EDIT_PRICE, hInst, NULL);

    CreateWindow("BUTTON", "Update", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP,
                 150, 135, 100, 30, g_hCurrentDialog, (HMENU)IDOK, hInst, NULL);
    CreateWindow("BUTTON", "Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                 270, 135, 100, 30, g_hCurrentDialog, (HMENU)IDCANCEL, hInst, NULL);

    ShowWindow(g_hCurrentDialog, SW_SHOW);
    SetFocus(g_hEditName);
    EnableWindow(hwnd, FALSE);

    MSG msg;
    while (g_hCurrentDialog != NULL && GetMessage(&msg, NULL, 0, 0)) {
        if (!IsDialogMessage(g_hCurrentDialog, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (g_dialogResult == IDOK) {
            break;
        }
    }

    if (g_dialogResult == IDOK) {
        char newName[256], qtyStr[50], priceStr[50];
        GetWindowText(g_hEditName, newName, 256);
        GetWindowText(g_hEditQty, qtyStr, 50);
        GetWindowText(g_hEditPrice, priceStr, 50);

        if (g_hCurrentDialog) {
            DestroyWindow(g_hCurrentDialog);
            g_hCurrentDialog = NULL;
        }

        EnableWindow(hwnd, TRUE);
        SetForegroundWindow(hwnd);

        if (strlen(newName) == 0) {
            MessageBox(hwnd, "Please enter product name", "Validation Error", MB_OK | MB_ICONWARNING);
            return;
        }

        int newQty = atoi(qtyStr);
        double newPrice = atof(priceStr);

        if (newPrice <= 0) {
            MessageBox(hwnd, "Price must be greater than 0", "Validation Error", MB_OK | MB_ICONWARNING);
            return;
        }

        const char* sql = "UPDATE products SET name = ?, quantity = ?, price = ? WHERE id = ?";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, newName, -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 2, newQty);
            sqlite3_bind_double(stmt, 3, newPrice);
            sqlite3_bind_int(stmt, 4, atoi(id));

            if (sqlite3_step(stmt) == SQLITE_DONE) {
                ShowSuccess("Product updated successfully!");
                LoadProducts();
            } else {
                ShowError("Failed to update product.");
            }
        }
        sqlite3_finalize(stmt);
    } else {
        if (g_hCurrentDialog) {
            DestroyWindow(g_hCurrentDialog);
            g_hCurrentDialog = NULL;
        }
        EnableWindow(hwnd, TRUE);
        SetForegroundWindow(hwnd);
    }
}
void ShowError(const char* message) {
    MessageBox(NULL, message, "Error", MB_OK | MB_ICONERROR);
}

void ShowSuccess(const char* message) {
    MessageBox(NULL, message, "Success", MB_OK | MB_ICONINFORMATION);
}

void DeleteProduct(HWND hwnd) {
    int selectedIndex = ListView_GetNextItem(hListViewProducts, -1, LVNI_SELECTED);
    if (selectedIndex == -1) {
        ShowError("Please select a product to delete.");
        return;
    }

    char id[20], name[256];
    ListView_GetItemText(hListViewProducts, selectedIndex, 0, id, sizeof(id));
    ListView_GetItemText(hListViewProducts, selectedIndex, 1, name, sizeof(name));

    char confirmMsg[512];
    sprintf(confirmMsg, "Are you sure you want to delete:\n%s?", name);

    int result = MessageBox(hwnd, confirmMsg, "Confirm Delete",
                           MB_YESNO | MB_ICONQUESTION);

    if (result == IDYES) {
        const char* sql = "DELETE FROM products WHERE id = ?";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, atoi(id));

            if (sqlite3_step(stmt) == SQLITE_DONE) {
                ShowSuccess("Product deleted successfully!");
                LoadProducts();
            } else {
                ShowError("Failed to delete product.");
            }
        }
        sqlite3_finalize(stmt);
    }
}

void PurchaseProduct(HWND hwnd) {
    int selectedIndex = ListView_GetNextItem(hListViewProducts, -1, LVNI_SELECTED);
    if (selectedIndex == -1) {
        ShowError("Please select a product to purchase.");
        return;
    }

    char id[20], name[256], qty[20], price[20];
    ListView_GetItemText(hListViewProducts, selectedIndex, 0, id, sizeof(id));
    ListView_GetItemText(hListViewProducts, selectedIndex, 1, name, sizeof(name));
    ListView_GetItemText(hListViewProducts, selectedIndex, 2, qty, sizeof(qty));
    ListView_GetItemText(hListViewProducts, selectedIndex, 3, price, sizeof(price));

    int currentQty = atoi(qty);
    double unitPrice = atof(price);

    if (currentQty <= 0) {
        ShowError("Product is out of stock!");
        return;
    }

    // Register dialog class
    static BOOL registered = FALSE;
    if (!registered) {
        WNDCLASSEX wc = {0};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.lpfnWndProc = SimpleDialogProc;
        wc.hInstance = hInst;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = "SimpleDialog";
        RegisterClassEx(&wc);
        registered = TRUE;
    }

    g_dialogResult = 0;

    g_hCurrentDialog = CreateWindowEx(
        WS_EX_DLGMODALFRAME,
        "SimpleDialog",
        "Purchase Product",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        (GetSystemMetrics(SM_CXSCREEN) - 400) / 2,
        (GetSystemMetrics(SM_CYSCREEN) - 200) / 2,
        400, 200,
        hwnd, NULL, hInst, NULL
    );

    char info[512];
    sprintf(info, "Product: %s\nAvailable: %d units\nPrice: K%.2f per unit",
            name, currentQty, unitPrice);

    CreateWindow("STATIC", info, WS_CHILD | WS_VISIBLE,
                 20, 20, 350, 60, g_hCurrentDialog, NULL, hInst, NULL);

    CreateWindow("STATIC", "Quantity:", WS_CHILD | WS_VISIBLE,
                 20, 90, 150, 20, g_hCurrentDialog, NULL, hInst, NULL);

    // Use the GLOBAL variable g_hEditPurchaseQty
    g_hEditPurchaseQty = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "1",
                                    WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER | WS_TABSTOP,
                                    180, 88, 190, 22, g_hCurrentDialog, (HMENU)IDC_EDIT_PURCHASE_QTY, hInst, NULL);

    CreateWindow("BUTTON", "Purchase", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP,
                 150, 130, 100, 30, g_hCurrentDialog, (HMENU)IDOK, hInst, NULL);
    CreateWindow("BUTTON", "Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                 270, 130, 100, 30, g_hCurrentDialog, (HMENU)IDCANCEL, hInst, NULL);

    ShowWindow(g_hCurrentDialog, SW_SHOW);
    SetFocus(g_hEditPurchaseQty);  // Focus on the global variable
    EnableWindow(hwnd, FALSE);

    MSG msg;
    while (g_hCurrentDialog != NULL && GetMessage(&msg, NULL, 0, 0)) {
        if (!IsDialogMessage(g_hCurrentDialog, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Check if OK was clicked
        if (g_dialogResult == IDOK) {
            break;
        }
    }

    // Process the result BEFORE destroying dialog
    if (g_dialogResult == IDOK) {
        char qtyStr[50];
        GetWindowText(g_hEditPurchaseQty, qtyStr, 50);  // Use global variable
        int purchaseQty = atoi(qtyStr);

        // Now destroy the dialog
        if (g_hCurrentDialog) {
            DestroyWindow(g_hCurrentDialog);
            g_hCurrentDialog = NULL;
        }

        EnableWindow(hwnd, TRUE);
        SetForegroundWindow(hwnd);

        if (purchaseQty <= 0) {
            MessageBox(hwnd, "Quantity must be greater than 0", "Validation Error", MB_OK | MB_ICONWARNING);
            return;
        }

        if (purchaseQty > currentQty) {
            MessageBox(hwnd, "Not enough stock available!", "Error", MB_OK | MB_ICONWARNING);
            return;
        }

        sqlite3_exec(db, "BEGIN TRANSACTION", 0, 0, 0);

        const char* updateSql = "UPDATE products SET quantity = quantity - ? WHERE id = ?";
        sqlite3_stmt* stmt;
        int success = 0;

        if (sqlite3_prepare_v2(db, updateSql, -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, purchaseQty);
            sqlite3_bind_int(stmt, 2, atoi(id));

            if (sqlite3_step(stmt) == SQLITE_DONE) {
                success = 1;
            }
        }
        sqlite3_finalize(stmt);

        if (success) {
            const char* insertSql = "INSERT INTO sales (product_id, product_name, quantity_sold, total_amount) VALUES (?, ?, ?, ?)";
            double totalAmount = purchaseQty * unitPrice;

            if (sqlite3_prepare_v2(db, insertSql, -1, &stmt, 0) == SQLITE_OK) {
                sqlite3_bind_int(stmt, 1, atoi(id));
                sqlite3_bind_text(stmt, 2, name, -1, SQLITE_TRANSIENT);
                sqlite3_bind_int(stmt, 3, purchaseQty);
                sqlite3_bind_double(stmt, 4, totalAmount);

                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    char successMsg[256];
                    sprintf(successMsg, "Purchase successful!\nTotal: K%.2f", totalAmount);
                    sqlite3_exec(db, "COMMIT", 0, 0, 0);
                    ShowSuccess(successMsg);
                    LoadProducts();
                } else {
                    sqlite3_exec(db, "ROLLBACK", 0, 0, 0);
                    ShowError("Failed to record sale.");
                }
            }
            sqlite3_finalize(stmt);
        } else {
            sqlite3_exec(db, "ROLLBACK", 0, 0, 0);
            ShowError("Failed to update inventory.");
        }
    } else {
        // Cancel was clicked
        if (g_hCurrentDialog) {
            DestroyWindow(g_hCurrentDialog);
            g_hCurrentDialog = NULL;
        }
        EnableWindow(hwnd, TRUE);
        SetForegroundWindow(hwnd);
    }
}

void SearchProducts(HWND hwnd) {
    char searchText[256];
    GetWindowText(hEditSearch, searchText, 256);

    if (strlen(searchText) == 0) {
        LoadProducts();
        return;
    }

    ListView_DeleteAllItems(hListViewProducts);

    char sql[512];
    sprintf(sql, "SELECT id, name, quantity, price, created_at FROM products WHERE name LIKE '%%%s%%' ORDER BY id", searchText);

    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) == SQLITE_OK) {
        int row = 0;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            LVITEM lvi = {0};
            char buffer[256];

            lvi.mask = LVIF_TEXT;
            lvi.iItem = row;

            sprintf(buffer, "%d", sqlite3_column_int(stmt, 0));
            lvi.pszText = buffer;
            ListView_InsertItem(hListViewProducts, &lvi);

            ListView_SetItemText(hListViewProducts, row, 1,
                (char*)sqlite3_column_text(stmt, 1));

            sprintf(buffer, "%d", sqlite3_column_int(stmt, 2));
            ListView_SetItemText(hListViewProducts, row, 2, buffer);

            sprintf(buffer, "%.2f", sqlite3_column_double(stmt, 3));
            ListView_SetItemText(hListViewProducts, row, 3, buffer);

            ListView_SetItemText(hListViewProducts, row, 4,
                (char*)sqlite3_column_text(stmt, 4));

            row++;
        }
    }
    sqlite3_finalize(stmt);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            CreateControls(hwnd);
            break;

        case WM_NOTIFY: {
            LPNMHDR pnmhdr = (LPNMHDR)lParam;
            if (pnmhdr->idFrom == ID_TAB_CONTROL && pnmhdr->code == TCN_SELCHANGE) {
                int tabIndex = TabCtrl_GetCurSel(hTabControl);
                if (tabIndex == 0) {
                    ShowWindow(hListViewProducts, SW_SHOW);
                    ShowWindow(hListViewSales, SW_HIDE);
                } else if (tabIndex == 1) {
                    ShowWindow(hListViewProducts, SW_HIDE);
                    ShowWindow(hListViewSales, SW_SHOW);
                    LoadSales();
                }
            }
            break;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_BTN_ADD:
                    AddProduct(hwnd);
                    break;
                case ID_BTN_UPDATE:
                    UpdateProduct(hwnd);
                    break;
                case ID_BTN_DELETE:
                    DeleteProduct(hwnd);
                    break;
                case ID_BTN_PURCHASE:
                    PurchaseProduct(hwnd);
                    break;
                case ID_BTN_REFRESH:
                    LoadProducts();
                    break;
                case ID_BTN_VIEW_SALES:
                    TabCtrl_SetCurSel(hTabControl, 1);
                    ShowWindow(hListViewProducts, SW_HIDE);
                    ShowWindow(hListViewSales, SW_SHOW);
                    LoadSales();
                    break;
                case ID_BTN_SEARCH:
                    SearchProducts(hwnd);
                    break;
            }
            break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}
