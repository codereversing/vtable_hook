#include <cstdio>
#include <Windows.h>

class Base
{
public:
    Base() { printf("-  Base::Base\n"); }
    virtual ~Base() { printf("-  Base::~Base\n"); }

    void A() { printf("-  Base::A\n"); }
    virtual void B() { printf("-  Base::B\n"); }
    virtual void C() { printf("-  Base::C\n"); }
};

class Derived final : public Base
{
public:
    Derived() { printf("-  Derived::Derived\n"); }
    ~Derived() { printf("-  Derived::~Derived\n"); }

    void B() override { printf("-  Derived::B\n"); }
    void C() override { printf("-  Derived::C\n"); }
};

void Invoke(Base * const pBase)
{
    pBase->A();
    pBase->B();
    pBase->C();
}

void PrintVTable(Base * const pBase)
{
    unsigned int *pVTableBase = (unsigned int *)(*(unsigned int *)pBase);
    printf("First: %p\n"
        "Second: %p\n"
        "Third: %p\n",
        *pVTableBase, *(pVTableBase + 1), *(pVTableBase + 2));
}

void __fastcall VMTHookFnc(void *pEcx, void *pEdx)
{
    Base *pThisPtr = (Base *)pEcx;

    printf("In VMTHookFnc\n");
}

void HookVMT(Base * const pBase)
{
    unsigned int *pVTableBase = (unsigned int *)(*(unsigned int *)pBase);
    unsigned int *pVTableFnc = (unsigned int *)((pVTableBase + 1));
    void *pHookFnc = (void *)VMTHookFnc;

    SIZE_T ulOldProtect = 0;
    (void)VirtualProtect(pVTableFnc, sizeof(void *), PAGE_EXECUTE_READWRITE, &ulOldProtect);
    memcpy(pVTableFnc, &pHookFnc, sizeof(void *));
    (void)VirtualProtect(pVTableFnc, sizeof(void *), ulOldProtect, &ulOldProtect);
}

int main(int argc, char *argv[])
{
    printf("Constructing Base object.\n");
    Base base;
    printf("Constructing Derived object on stack.\n");
    Derived derived;
    printf("Constructing Derived object on heap.\n");
    Base *pBase = new Derived;

    printf("Printing VTable for Base object.\n");
    PrintVTable(&base);
    printf("Printing VTable for Derived object.\n");
    PrintVTable(&derived);
    printf("Printing VTable for (heap) Derived object.\n");
    PrintVTable(pBase);

    printf("Invoking functions on Base object.\n");
    Invoke(&base);
    printf("Invoking functions on Derived object.\n");
    Invoke(&derived);
    printf("Invoking functions on Derived object.\n");
    Invoke(pBase);

    printf("Hooking (heap) Derived::B\n");
    HookVMT(pBase);
    printf("Invoking functions on (heap )Derived object.\n");
    Invoke(pBase);

    printf("Deleting (heap) Derived object.\n");
    delete pBase;

    getchar();
    return 0;
}