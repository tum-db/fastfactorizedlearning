# Fast Factorized Learning: Powered by In-Memory Database Systems
Accompanying code to the submission to the above mentioned paper.
Code is PostgreSQL compatible.

## Configuration
Change the first line in test.cpp to modify the PostgreSQL connection:
```
const std::string g_favorita{"dbname=Favorita hostaddr=127.0.0.1 port=5433"};
const std::string g_sales{"dbname=SalesWithNumbers hostaddr=127.0.0.1 port=5433"};
```

## Usage:
```
make
./test
```
