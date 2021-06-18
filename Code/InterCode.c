#include "InterCode.h"

InterCode head = NULL;
InterCode tail = NULL;

int varNo = 1;
int labelNo = 1;

void insertInterCode(InterCode i) {
    i->prev = NULL;
    i->next = NULL;
    if (head) {
        tail->next = i;
        i->prev = tail;
        tail = i;
    } else {
        head = i;
        tail = i;
    }
}

void deleteInterCode(InterCode i) {
    if (head == i && tail == i) {
        head = NULL;
        tail == NULL;
    } else if (head == i) {
        if (i->next) i->next->prev = NULL;
        head = i->next;
    }
    // ... 暂时没用 优化代码时才有用
}

void printOperand(Operand o) {
    if (!o) {
        return;
    }
    switch (o->kind) {
        case VAR:
            printf("%s", o->u.value);
            break;
        case CONSTANT_OP:
            printf("#%s", o->u.value);
            break;
        case VAR_ADDRESS:
            printf("*%s", o->u.var->u.value);
            break;
        case LABEL_OP:
            printf("label%d", o->u.var_no);
            break;
        case FUNCTION_OP:
            printf("%s", o->u.value);
            break;
        case TMP_VAR:
            printf("t%d", o->u.var_no);
            break;
        case TMP_VAR_ADDRESS:
            printf("*t%d", o->u.var->u.var_no);
            break;
    }
}

void printInterCode(void) {
    InterCode tmp = head;
    while (tmp) {
        switch (tmp->kind) {
            case LABEL:
                printf("LABEL ");
                printOperand(tmp->u.unary.op);
                printf(" :");
                break;
            case FUNCTION:
                printf("FUNCTION ");
                printOperand(tmp->u.unary.op);
                printf(" :");
                break;
            case ASSIGN:
                printOperand(tmp->u.assign.left);
                printf(" := ");
                printOperand(tmp->u.assign.right);
                break;
            case ADD_KIND:
                printOperand(tmp->u.binop.result);
                printf(" := ");
                printOperand(tmp->u.binop.op1);
                printf(" + ");
                printOperand(tmp->u.binop.op2);
                break;
            case SUB_KIND:
                printOperand(tmp->u.binop.result);
                printf(" := ");
                printOperand(tmp->u.binop.op1);
                printf(" - ");
                printOperand(tmp->u.binop.op2);
                break;
            case MUL_KIND:
                printOperand(tmp->u.binop.result);
                printf(" := ");
                printOperand(tmp->u.binop.op1);
                printf(" * ");
                printOperand(tmp->u.binop.op2);
                break;
            case DIV_KIND:
                printOperand(tmp->u.binop.result);
                printf(" := ");
                printOperand(tmp->u.binop.op1);
                printf(" / ");
                printOperand(tmp->u.binop.op2);
                break;
            case RIGHTAT:
                printOperand(tmp->u.assign.left);
                printf(" := &");
                printOperand(tmp->u.assign.right);
                break;
            case GOTO:
                printf("GOTO ");
                printOperand(tmp->u.unary.op);
                break;
            case IFGOTO:
                printf("IF ");
                printOperand(tmp->u.ifgoto.t1);
                printf(" %s ", tmp->u.ifgoto.op);
                printOperand(tmp->u.ifgoto.t2);
                printf(" GOTO ");
                printOperand(tmp->u.ifgoto.label);
                break;
            case RETURN_KIND:
                printf("RETURN ");
                printOperand(tmp->u.unary.op);
                break;
            case DEC:
                printf("DEC ");
                printOperand(tmp->u.dec.op);
                printf(" %d", tmp->u.dec.size);
                break;
            case ARG:
                printf("ARG ");
                printOperand(tmp->u.unary.op);
                break;
            case CALL:
                printOperand(tmp->u.assign.left);
                printf(" := CALL ");
                printOperand(tmp->u.assign.right);
                break;
            case PARAM:
                printf("PARAM ");
                printOperand(tmp->u.unary.op);
                break;
            case READ:
                printf("READ ");
                printOperand(tmp->u.unary.op);
                break;
            case WRITE:
                printf("WRITE ");
                printOperand(tmp->u.unary.op);
                break;
        }
        printf("\n");
        tmp = tmp->next;
    }
}