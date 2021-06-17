/*
 * Based on arch/arm/kernel/traps.c
 *
 * Copyright (C) 1995-2009 Russell King
 * Copyright (C) 2012 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <linux/kernel.h>

#include <asm/pgtable.h>

void __pte_error(const char *file, int line, u64 val)
{
	pr_err("%s:%d: bad pte %016llx.\n", file, line, val);
}

void __pmd_error(const char *file, int line, u64 val)
{
	pr_err("%s:%d: bad pmd %016llx.\n", file, line, val);
}

void __pud_error(const char *file, int line, u64 val)
{
	pr_err("%s:%d: bad pud %016llx.\n", file, line, val);
}

void __pgd_error(const char *file, int line, u64 val)
{
	pr_err("%s:%d: bad pgd %016llx.\n", file, line, val);
}

