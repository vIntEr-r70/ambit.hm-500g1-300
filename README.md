```mermaid
flowchart LR
A[create] --> B[start timer]
B --> D{is_allow?}
D -- No --> D
D -- Yes --> E[activate]
E --> F[proc]
F --> G{is_done?}
G -- No --> F
G -- Yes --> H[deactivate]
H --> D



```