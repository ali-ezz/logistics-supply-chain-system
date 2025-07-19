# 🚚 Logistics and Supply Chain System  
**Secure role-based command-line system for supply chain management**  

## 📌 About the Project  
This system implements a complete logistics management solution with:  
- Three distinct user roles (Admin, Warehouse, Customer)  
- Isolated directory structure with permission controls  
- Full suite of file operations (list, copy, move, permissions)  
- Symbolic links for shared resources  
- Order note-taking via redirection operators  

## 🌟 Key Features  
| Operation            | Admin | Warehouse | Customer |  
|----------------------|-------|-----------|----------|  
| List files           | ✅     | ✅        | ✅       |  
| Change permissions   | ✅     | ❌        | ❌       |  
| Create/delete files  | ✅     | ✅        | ❌       |  
| Symbolic links       | ✅     | ❌        | ❌       |  
| Copy files           | ✅     | ❌        | ✅       |  
| Move shipments       | ❌     | ✅        | ❌       |  
| Order notes (>>)     | ❌     | ❌        | ✅       |  

## 📂 Directory Structure  
```plaintext
/logistics
├── admin/           # inventory.txt, delivery_schedules.db
├── warehouse/       # stock.dat, shipment_logs/
└── customers/       # order_123.track, order_456.track
```

## ⚙️ Quick Start  
```bash
# 1. Clone repository
git clone https://github.com/yourusername/logistics-supply-chain-system

# 2. Compile program
cd logistics-supply-chain-system
gcc logistics.c -o logistics_system

# 3. Run system
./logistics_system

# 4. Login with:
#    - admin (full access)
#    - wh1 (warehouse access)
#    - cust1 (customer access)
```

## 🛠️ Technical Implementation  
```c
// Role-based access control
typedef enum { ADMIN, WAREHOUSE, CUSTOMER } UserType;

// Secure directory isolation
char path[MAX_PATH];
snprintf(path, sizeof(path), "%s/%s", user->home_dir, filename);

// Permission validation
if (user->type != ADMIN) {
    printf("Permission denied\n");
    return;
}
```

## 🚀 Future Roadmap  
- Database persistence with SQLite  
- Network socket implementation  
- Web-based GUI interface  
- Automated report generation  
