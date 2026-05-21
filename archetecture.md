graph TD
    %% Styling for a modern dark theme aesthetic
    classDef default fill:#161b22,stroke:#30363d,stroke-width:2px,color:#c9d1d9,font-family:monospace,rx:5px,ry:5px;
    classDef user fill:#0d1117,stroke:#58a6ff,stroke-width:2px,color:#58a6ff,rx:5px,ry:5px;
    classDef core fill:#0d1117,stroke:#2ea043,stroke-width:2px,color:#2ea043,rx:5px,ry:5px;
    classDef output fill:#0d1117,stroke:#bc8cff,stroke-width:2px,color:#bc8cff,rx:5px,ry:5px;

    User["User (CLI)"]:::user --> Core["MarketPulse (C Program)"]:::core
    
    Core --> Net["Network Module<br/>(socket/SSL)"]
    Core --> Mon["Monitor Engine<br/>(fork/pipe)"]
    Core --> AI["AI Module<br/>(Groq API)"]
    
    Net --> Parse["Data Parser<br/>(JSON)"]
    Mon --> Alert["Alert Engine<br/>(signal)"]
    AI --> Gen["Insight Gen<br/>(Llama 3.3)"]
    
    Parse --> Term["Terminal Display"]:::output
    Alert --> Term
    Gen --> Term
